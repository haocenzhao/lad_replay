#include "plot_common.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TClass.h"
#include "TKey.h"
#include "TH1D.h"
#include "TMarker.h"
#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cmath>
void calc_timing_windows(TString, TString, TString, TString, Double_t, Double_t[], Double_t[], bool,
                         double);

// Save the same page twice; detailed changes only the horizontal display range.
void save_timing_window_views(TCanvas *canvas, TFile *output) {
  TDirectory *original = output->GetDirectory("original");
  if (!original)
    original = output->mkdir("original");
  original->cd();
  canvas->Write(canvas->GetName(), TObject::kOverwrite);
  if (!ladplot::flag("DETAILED", true)) {
    output->cd();
    return;
  }
  TDirectory *detailed = output->GetDirectory("detailed");
  if (!detailed)
    detailed = output->mkdir("detailed");
  original->cd();
  canvas->Write(canvas->GetName(), TObject::kOverwrite);
  TCanvas *zoom = static_cast<TCanvas *>(canvas->Clone(TString(canvas->GetName()) + "_detailed"));
  TString name = canvas->GetName();
  for (int index = 1; index <= 16; ++index) {
    TPad *pad = dynamic_cast<TPad *>(zoom->GetPad(index));
    if (!pad)
      continue;
    TH1 *hist = nullptr;
    TMarker *peak = nullptr;
    TIter objects(pad->GetListOfPrimitives());
    TObject *object;
    while ((object = objects())) {
      if (object->InheritsFrom(TH1::Class()))
        hist = static_cast<TH1 *>(object);
      if (object->InheritsFrom(TMarker::Class()))
        peak = static_cast<TMarker *>(object);
    }
    if (!hist)
      continue;
    double low, high;
    if (name.BeginsWith("hcer_")) {
      low = ladplot::number("HCER_DETAIL_MIN", -250.);
      high = ladplot::number("HCER_DETAIL_MAX", 350.);
    } else if (name.BeginsWith("phgcer_") || name.BeginsWith("pngcer_")) {
      low = ladplot::number("PCER_DETAIL_MIN", -400.);
      high = ladplot::number("PCER_DETAIL_MAX", 200.);
    } else {
      double center = peak ? peak->GetX() : hist->GetBinCenter(hist->GetMaximumBin());
      // Empty channels have no measured peak: use the cut midpoint as a display fallback.
      if (!peak && hist->Integral() <= 0) {
        std::vector<double> cuts;
        TIter lines(pad->GetListOfPrimitives());
        while ((object = lines()))
          if (object->InheritsFrom(TLine::Class()))
            cuts.push_back(static_cast<TLine *>(object)->GetX1());
        if (cuts.size() == 2)
          center = (cuts[0] + cuts[1]) / 2.;
      }
      low = center - ladplot::number("DETAIL_HALF_WIDTH", 40.);
      high = center + ladplot::number("DETAIL_HALF_WIDTH", 40.);
    }
    // Pad the detailed view with empty bins if its requested range exceeds the source axis.
    const int oldBins = hist->GetNbinsX();
    const double binWidth = hist->GetXaxis()->GetBinWidth(1);
    const double oldLow = hist->GetXaxis()->GetXmin(), oldHigh = hist->GetXaxis()->GetXmax();
    if (low < oldLow || high > oldHigh) {
      std::vector<double> contents(oldBins + 2), errors(oldBins + 2);
      for (int bin = 0; bin <= oldBins + 1; ++bin) {
        contents[bin] = hist->GetBinContent(bin);
        errors[bin] = hist->GetBinError(bin);
      }
      double entries = hist->GetEntries(), stats[4];
      hist->GetStats(stats);
      int left = low < oldLow ? int(std::ceil((oldLow - low) / binWidth)) : 0;
      int right = high > oldHigh ? int(std::ceil((high - oldHigh) / binWidth)) : 0;
      hist->SetBins(oldBins + left + right, oldLow - left * binWidth, oldHigh + right * binWidth);
      for (int bin = 1; bin <= oldBins; ++bin) {
        hist->SetBinContent(bin + left, contents[bin]);
        hist->SetBinError(bin + left, errors[bin]);
      }
      hist->SetBinContent(0, contents[0]);
      hist->SetBinError(0, errors[0]);
      hist->SetBinContent(hist->GetNbinsX() + 1, contents[oldBins + 1]);
      hist->SetBinError(hist->GetNbinsX() + 1, errors[oldBins + 1]);
      hist->PutStats(stats);
      hist->SetEntries(entries);
    }
    hist->GetXaxis()->SetRangeUser(low, high);
    pad->Modified();
  }
  zoom->Modified();
  detailed->cd();
  zoom->Write(canvas->GetName(), TObject::kOverwrite);
  delete zoom;
  output->cd();
}

void run_shms_timing_windows(TString file_name, TString out_file, int RunNumber,
                             bool newWindows = false, double width = 40., TString spec = "shms") {
  gROOT->SetBatch(kTRUE); // do not display plots
  gHcParms->Define("gen_run_number", "Run Number", RunNumber);
  if (spec.CompareTo("shms", TString::kIgnoreCase) == 0) {
    gHcParms->AddString(
        "g_ctp_database_filename",
        ladplot::setting("PLOT_DATABASE", "DBASE/LAD_COIN/standard.database").Data());
    gHcParms->Load(gHcParms->GetString("g_ctp_database_filename"), RunNumber);
    gHcParms->Load(gHcParms->GetString("g_ctp_parm_filename"));
    ladplot::overrideParameters();
    gHcParms->Load(gHcParms->GetString("g_ctp_kinematics_filename"), RunNumber);
    // Load parameters for SHMS trigger configuration
    gHcParms->Load("PARAM/TRIG/tshms.param");
  } else if (spec.CompareTo("coin", TString::kIgnoreCase) == 0) {
    gHcParms->AddString(
        "g_ctp_database_filename",
        ladplot::setting("PLOT_DATABASE", "DBASE/LAD_COIN/standard.database").Data());
    gHcParms->Load(gHcParms->GetString("g_ctp_database_filename"), RunNumber);
    gHcParms->Load(gHcParms->GetString("g_ctp_parm_filename"));
    ladplot::overrideParameters();
    gHcParms->Load(gHcParms->GetString("g_ctp_kinematics_filename"), RunNumber);
    // Load params for COIN trigger configuration
    gHcParms->Load("PARAM/TRIG/tcoin.param");
    // Load fadc debug parameters
    gHcParms->Load("PARAM/HMS/GEN/h_fadc_debug.param");
    gHcParms->Load("PARAM/SHMS/GEN/p_fadc_debug.param");
  } else {
    cout << "Unknown Spec: " << spec << endl;
    return;
  }

  const char *prefix = "p";

  UInt_t nPMT = 21;                  // 2Y plane has 21 PMTs
  UInt_t nPlanes = 4;                // 1x, 1y, 2x, 2y
  UInt_t fHodoScin = nPlanes * nPMT; // This could be determined from hcana DB
  Double_t *fHodoPosAdcTimeWindowMin = new Double_t[fHodoScin];
  Double_t *fHodoPosAdcTimeWindowMax = new Double_t[fHodoScin];
  Double_t *fHodoNegAdcTimeWindowMin = new Double_t[fHodoScin];
  Double_t *fHodoNegAdcTimeWindowMax = new Double_t[fHodoScin];

  UInt_t fPrShwr = 14;
  Double_t *fCaloPosAdcTimeWindowMin = new Double_t[fPrShwr];
  Double_t *fCaloPosAdcTimeWindowMax = new Double_t[fPrShwr];
  Double_t *fCaloNegAdcTimeWindowMin = new Double_t[fPrShwr];
  Double_t *fCaloNegAdcTimeWindowMax = new Double_t[fPrShwr];

  UInt_t fFlyShwr = 224;
  Double_t *fCaloArrAdcTimeWindowMin = new Double_t[fFlyShwr];
  Double_t *fCaloArrAdcTimeWindowMax = new Double_t[fFlyShwr];

  UInt_t fNGCer = 4;
  Double_t *fNGCerAdcTimeWindowMin = new Double_t[fNGCer];
  Double_t *fNGCerAdcTimeWindowMax = new Double_t[fNGCer];

  UInt_t fHGCer = 4;
  Double_t *fHGCerAdcTimeWindowMin = new Double_t[fHGCer];
  Double_t *fHGCerAdcTimeWindowMax = new Double_t[fHGCer];

  DBRequest windowList[] = {
      {"hodo_PosAdcTimeWindowMin", fHodoPosAdcTimeWindowMin, kDouble, fHodoScin, 1},
      {"hodo_PosAdcTimeWindowMax", fHodoPosAdcTimeWindowMax, kDouble, fHodoScin, 1},
      {"hodo_NegAdcTimeWindowMin", fHodoNegAdcTimeWindowMin, kDouble, fHodoScin, 1},
      {"hodo_NegAdcTimeWindowMax", fHodoNegAdcTimeWindowMax, kDouble, fHodoScin, 1},
      {"cal_pos_AdcTimeWindowMin", fCaloPosAdcTimeWindowMin, kDouble, fPrShwr, 1},
      {"cal_pos_AdcTimeWindowMax", fCaloPosAdcTimeWindowMax, kDouble, fPrShwr, 1},
      {"cal_neg_AdcTimeWindowMin", fCaloNegAdcTimeWindowMin, kDouble, fPrShwr, 1},
      {"cal_neg_AdcTimeWindowMax", fCaloNegAdcTimeWindowMax, kDouble, fPrShwr, 1},
      {"cal_arr_AdcTimeWindowMin", fCaloArrAdcTimeWindowMin, kDouble, fFlyShwr, 1},
      {"cal_arr_AdcTimeWindowMax", fCaloArrAdcTimeWindowMax, kDouble, fFlyShwr, 1},
      {"ngcer_adcTimeWindowMin", fNGCerAdcTimeWindowMin, kDouble, fNGCer, 1},
      {"ngcer_adcTimeWindowMax", fNGCerAdcTimeWindowMax, kDouble, fNGCer, 1},
      {"hgcer_adcTimeWindowMin", fHGCerAdcTimeWindowMin, kDouble, fHGCer, 1},
      {"hgcer_adcTimeWindowMax", fHGCerAdcTimeWindowMax, kDouble, fHGCer, 1},
      {0},
  };

  gHcParms->LoadParmValues((DBRequest *)&windowList, prefix);

  calc_timing_windows(file_name, out_file, "hodo_1x", "p", 1, fHodoPosAdcTimeWindowMin,
                      fHodoPosAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hodo_1x", "p", 2, fHodoNegAdcTimeWindowMin,
                      fHodoNegAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hodo_1y", "p", 1, fHodoPosAdcTimeWindowMin,
                      fHodoPosAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hodo_1y", "p", 2, fHodoNegAdcTimeWindowMin,
                      fHodoNegAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hodo_2x", "p", 1, fHodoPosAdcTimeWindowMin,
                      fHodoPosAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hodo_2x", "p", 2, fHodoNegAdcTimeWindowMin,
                      fHodoNegAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hodo_2y", "p", 1, fHodoPosAdcTimeWindowMin,
                      fHodoPosAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hodo_2y", "p", 2, fHodoNegAdcTimeWindowMin,
                      fHodoNegAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "cal_prshwr", "p", 1, fCaloPosAdcTimeWindowMin,
                      fCaloPosAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "cal_prshwr", "p", 2, fCaloNegAdcTimeWindowMin,
                      fCaloNegAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "cal_shwr", "p", 0, fCaloArrAdcTimeWindowMin,
                      fCaloArrAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hgcer", "p", 0, fHGCerAdcTimeWindowMin,
                      fHGCerAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "ngcer", "p", 0, fNGCerAdcTimeWindowMin,
                      fNGCerAdcTimeWindowMax, newWindows, width);
}

void run_hms_timing_windows(TString file_name, TString out_file, int RunNumber,
                            bool newWindows = false, double width = 40., TString spec = "hms") {
  gROOT->SetBatch(kTRUE); // do not display plots
  // Load Global parameters
  // Add variables to global list.
  gHcParms->Define("gen_run_number", "Run Number", RunNumber);
  if (spec.CompareTo("hms", TString::kIgnoreCase) == 0) {
    gHcParms->AddString(
        "g_ctp_database_filename",
        ladplot::setting("PLOT_DATABASE", "DBASE/LAD_COIN/standard.database").Data());
    gHcParms->Load(gHcParms->GetString("g_ctp_database_filename"), RunNumber);
    gHcParms->Load(gHcParms->GetString("g_ctp_parm_filename"));
    ladplot::overrideParameters();
    gHcParms->Load(gHcParms->GetString("g_ctp_kinematics_filename"), RunNumber);
    // Load params for HMS trigger configuration
    gHcParms->Load("PARAM/TRIG/thms.param");
    // Load fadc debug parameters
    gHcParms->Load("PARAM/HMS/GEN/h_fadc_debug.param");
  } else if (spec.CompareTo("coin", TString::kIgnoreCase) == 0) {
    gHcParms->AddString(
        "g_ctp_database_filename",
        ladplot::setting("PLOT_DATABASE", "DBASE/LAD_COIN/standard.database").Data());
    gHcParms->Load(gHcParms->GetString("g_ctp_database_filename"), RunNumber);
    gHcParms->Load(gHcParms->GetString("g_ctp_parm_filename"));
    ladplot::overrideParameters();
    gHcParms->Load(gHcParms->GetString("g_ctp_kinematics_filename"), RunNumber);
    // Load params for COIN trigger configuration
    gHcParms->Load("PARAM/TRIG/tcoin.param");
    // Load fadc debug parameters
    gHcParms->Load("PARAM/HMS/GEN/h_fadc_debug.param");
    gHcParms->Load("PARAM/SHMS/GEN/p_fadc_debug.param");
  } else {
    cout << "Unknown Spec: " << spec << endl;
    return;
  }

  const char *prefix = "h";

  UInt_t nPMT = 16;                  // 2Y plane has 21 PMTs
  UInt_t nPlanes = 4;                // 1x, 1y, 2x, 2y
  UInt_t fHodoScin = nPlanes * nPMT; // This could be determined from hcana DB
  Double_t *fHodoPosAdcTimeWindowMin = new Double_t[fHodoScin];
  Double_t *fHodoPosAdcTimeWindowMax = new Double_t[fHodoScin];
  Double_t *fHodoNegAdcTimeWindowMin = new Double_t[fHodoScin];
  Double_t *fHodoNegAdcTimeWindowMax = new Double_t[fHodoScin];

  UInt_t fPrShwr = 13 * 4;
  UInt_t fShwr = 13 * 4;
  Double_t *fCaloPosAdcTimeWindowMin = new Double_t[fPrShwr];
  Double_t *fCaloPosAdcTimeWindowMax = new Double_t[fPrShwr];
  Double_t *fCaloNegAdcTimeWindowMin = new Double_t[fShwr];
  Double_t *fCaloNegAdcTimeWindowMax = new Double_t[fShwr];

  UInt_t fCer = 2;
  Double_t *fCerAdcTimeWindowMin = new Double_t[fCer];
  Double_t *fCerAdcTimeWindowMax = new Double_t[fCer];

  DBRequest windowList[] = {
      {"hodo_PosAdcTimeWindowMin", fHodoPosAdcTimeWindowMin, kDouble, fHodoScin, 1},
      {"hodo_PosAdcTimeWindowMax", fHodoPosAdcTimeWindowMax, kDouble, fHodoScin, 1},
      {"hodo_NegAdcTimeWindowMin", fHodoNegAdcTimeWindowMin, kDouble, fHodoScin, 1},
      {"hodo_NegAdcTimeWindowMax", fHodoNegAdcTimeWindowMax, kDouble, fHodoScin, 1},
      {"cal_pos_AdcTimeWindowMin", fCaloPosAdcTimeWindowMin, kDouble, fPrShwr, 1},
      {"cal_pos_AdcTimeWindowMax", fCaloPosAdcTimeWindowMax, kDouble, fPrShwr, 1},
      {"cal_neg_AdcTimeWindowMin", fCaloNegAdcTimeWindowMin, kDouble, fShwr, 1},
      {"cal_neg_AdcTimeWindowMax", fCaloNegAdcTimeWindowMax, kDouble, fShwr, 1},
      {"cer_adcTimeWindowMin", fCerAdcTimeWindowMin, kDouble, fCer, 1},
      {"cer_adcTimeWindowMax", fCerAdcTimeWindowMax, kDouble, fCer, 1},
      {0},
  };

  gHcParms->LoadParmValues((DBRequest *)&windowList, prefix);
  calc_timing_windows(file_name, out_file, "hodo_1x", "h", 1, fHodoPosAdcTimeWindowMin,
                      fHodoPosAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hodo_1x", "h", 2, fHodoNegAdcTimeWindowMin,
                      fHodoNegAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hodo_1y", "h", 1, fHodoPosAdcTimeWindowMin,
                      fHodoPosAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hodo_1y", "h", 2, fHodoNegAdcTimeWindowMin,
                      fHodoNegAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hodo_2x", "h", 1, fHodoPosAdcTimeWindowMin,
                      fHodoPosAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hodo_2x", "h", 2, fHodoNegAdcTimeWindowMin,
                      fHodoNegAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hodo_2y", "h", 1, fHodoPosAdcTimeWindowMin,
                      fHodoPosAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "hodo_2y", "h", 2, fHodoNegAdcTimeWindowMin,
                      fHodoNegAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "cer", "h", 0, fCerAdcTimeWindowMin,
                      fCerAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "cal_hA", "h", 1, fCaloPosAdcTimeWindowMin,
                      fCaloPosAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "cal_hB", "h", 1, fCaloPosAdcTimeWindowMin,
                      fCaloPosAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "cal_hC", "h", 1, fCaloPosAdcTimeWindowMin,
                      fCaloPosAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "cal_hD", "h", 1, fCaloPosAdcTimeWindowMin,
                      fCaloPosAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "cal_hA", "h", 2, fCaloNegAdcTimeWindowMin,
                      fCaloNegAdcTimeWindowMax, newWindows, width);
  calc_timing_windows(file_name, out_file, "cal_hB", "h", 2, fCaloNegAdcTimeWindowMin,
                      fCaloNegAdcTimeWindowMax, newWindows, width);
}

void run_coin_timing_windows(TString file_name, TString out_file, int RunNumber,
                             bool newWindows = false, double width = 40.) {
  run_shms_timing_windows(file_name, out_file, RunNumber, newWindows, width, "coin");
  run_hms_timing_windows(file_name, out_file, RunNumber, newWindows, width, "coin");
}

void calc_timing_windows(TString golden_file = "", TString out_file = "", TString detector = "",
                         TString spect = "", Double_t polarity = -1, Double_t *minArr = 0,
                         Double_t *maxArr = 0, bool newWindows = false, double width = 40.) {
  if (golden_file == "") {
    cout << "Enter golden run root file name: " << endl;
    cin >> golden_file;
  }
  if (out_file == "") {
    cout << "Enter golden run root file name: " << endl;
    cin >> out_file;
  }
  if (detector == "") {
    cout << "Enter detector prefix (hodo_1x (etc.), hgcer, aero, cal_prshwr, "
            "cal_shwr, ngcer, cal_hA (etc.)): "
         << endl;
    cin >> detector;
  }
  if (spect == "") {
    cout << "Enter a spectrometer (p or h): " << endl;
    cin >> spect;
  }
  if (polarity == -1) {
    cout << "Enter a detector polarity (pos = 1, neg = 2): " << endl;
    cin >> polarity;
  }

  UInt_t offset;
  TString histname = Form("%s%s", spect.Data(), detector.Data());
  if (histname.Contains("hodo") && histname.Contains("1x") && polarity == 1) {
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_pos");
    offset = 0;
  }
  if (histname.Contains("hodo") && histname.Contains("1x") && polarity == 2) {
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_neg");
    offset = 0;
  }
  if (histname.Contains("hodo") && histname.Contains("1y") && polarity == 1) {
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_pos");
    offset = 1;
  }
  if (histname.Contains("hodo") && histname.Contains("1y") && polarity == 2) {
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_neg");
    offset = 1;
  }
  if (histname.Contains("hodo") && histname.Contains("2x") && polarity == 1) {
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_pos");
    offset = 2;
  }
  if (histname.Contains("hodo") && histname.Contains("2x") && polarity == 2) {
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_neg");
    offset = 2;
  }
  if (histname.Contains("hodo") && histname.Contains("2y") && polarity == 1) {
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_pos");
    offset = 3;
  }
  if (histname.Contains("hodo") && histname.Contains("2y") && polarity == 2) {
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_neg");
    offset = 3;
  }
  if (histname.Contains("aero") && polarity == 1)
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_pos");
  if (histname.Contains("aero") && polarity == 2)
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_neg");
  if (histname.Contains("hcer"))
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt");
  if (histname.Contains("hgcer"))
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt");
  if (histname.Contains("ngcer"))
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt");
  if (histname.Contains("_prshwr") && polarity == 1)
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_pos");
  if (histname.Contains("_prshwr") && polarity == 2)
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_neg");
  if (histname.Contains("_shwr"))
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt");
  if (histname.Contains("hcal") && histname.Contains("hA") && polarity == 1) {
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_pos");
    offset = 0;
  }
  if (histname.Contains("hcal") && histname.Contains("hB") && polarity == 1) {
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_pos");
    offset = 13;
  }
  if (histname.Contains("hcal") && histname.Contains("hC") && polarity == 1) {
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_pos");
    offset = 26;
  }
  if (histname.Contains("hcal") && histname.Contains("hD") && polarity == 1) {
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_pos");
    offset = 39;
  }
  if (histname.Contains("hcal") && histname.Contains("hA") && polarity == 2) {
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_neg");
    offset = 0;
  }
  if (histname.Contains("hcal") && histname.Contains("hB") && polarity == 2) {
    histname = Form("%s%s", histname.Data(), "_good_adctdc_diff_time_vs_pmt_neg");
    offset = 13;
  }

  TH2F *H1_adctdc_diff_time_vs_pmt;

  TFile *f1 = new TFile(golden_file, "READ");
  if (f1->IsZombie()) {
    cout << "Cannot find : " << golden_file << endl;
    return;
  }
  cout << histname.Data() << endl;
  f1->GetObject(histname.Data(), H1_adctdc_diff_time_vs_pmt);

  TFile *f2 = new TFile(out_file, "Update");
  if (f2->IsZombie()) {
    cout << "Cannot find : " << out_file << endl;
    return;
  }

  TH1D *H1_pmt;
  TCanvas *currentCanvas = nullptr;
  H1_pmt = (TH1D *)H1_adctdc_diff_time_vs_pmt->ProjectionX(
      "H1_pmt", 1, H1_adctdc_diff_time_vs_pmt->GetSize() - 2);

  TH1D *H1_adctdc_diff_time[H1_pmt->GetSize() - 2];

  for (Int_t ipmt = 0; ipmt < (H1_pmt->GetSize() - 2); ipmt++) {
    H1_adctdc_diff_time[ipmt] = (TH1D *)H1_adctdc_diff_time_vs_pmt->ProjectionY(
        Form("H1_adctdc_diff_time_pmt%d", ipmt + 1), ipmt + 1, ipmt + 1);
  }

  Double_t H1_adctdc_diff_time_peak[H1_pmt->GetSize() - 2];
  Double_t H1_adctdc_diff_time_fitpeak[H1_pmt->GetSize() - 2];
  for (Int_t ipmt = 0; ipmt < (H1_pmt->GetSize() - 2); ipmt++) {

    if (H1_adctdc_diff_time[ipmt]->GetEntries() > ladplot::number("PEAK_MIN_ENTRIES", 1000)) {
      TSpectrum *s = new TSpectrum(1);
      gSystem->RedirectOutput("/dev/null", "a");
      s->Search(H1_adctdc_diff_time[ipmt], ladplot::number("PEAK_SEARCH_SIGMA", 1.0),
                "nobackground&&nodraw", ladplot::number("PEAK_SEARCH_THRESHOLD", 0.001));
      gSystem->RedirectOutput(0);
      TList *functions = H1_adctdc_diff_time[ipmt]->GetListOfFunctions();
      TPolyMarker *pm = (TPolyMarker *)functions->FindObject("TPolyMarker");
      if (pm) {
        H1_adctdc_diff_time_peak[ipmt] = *pm->GetX();
        functions->Remove(pm);
        delete pm;
      } else {
        H1_adctdc_diff_time_peak[ipmt] = 0;
      }
    } else {
      H1_adctdc_diff_time_peak[ipmt] = 0;
    }

    // Low-statistics SHMS calorimeter peaks still have a valid maximum-bin position.
    if (spect == "p" && histname.Contains("cal") && H1_adctdc_diff_time_peak[ipmt] == 0 &&
        H1_adctdc_diff_time[ipmt]->Integral() > 0)
      H1_adctdc_diff_time_peak[ipmt] =
          H1_adctdc_diff_time[ipmt]->GetBinCenter(H1_adctdc_diff_time[ipmt]->GetMaximumBin());
    TLine *minLine, *maxLine;
    double ymax, maxVal, minVal;
    Int_t nCanvas = ipmt / 16 + 1;
    TString side = "";
    if (polarity == 1)
      side = "_pos";
    if (polarity == 2)
      side = "_neg";
    TString canvasName =
        Form("%s%s%s%s%d", spect.Data(), detector.Data(), side.Data(), "_diff_time_", nCanvas);
    if (ipmt % 16 == 0) {
      f2->cd();
      if (currentCanvas != NULL) {
        save_timing_window_views(currentCanvas, f2);
      }
      currentCanvas = new TCanvas(canvasName, canvasName, 1600, 1600);
      f1->cd();
      currentCanvas->Divide(4, 4, 0.003, 0.003);
    }
    int pad = 1 + (ipmt % 16);
    currentCanvas->cd(pad);
    gPad->SetLeftMargin(0.17);
    gPad->SetRightMargin(0.05);
    gPad->SetBottomMargin(0.18);
    gPad->SetTopMargin(0.12);
    TString oldTitle = H1_adctdc_diff_time[ipmt]->GetTitle();
    TString subTitle = oldTitle(0, oldTitle.Length() - 14);
    TString newTitle = Form("%s%s%d", subTitle.Data(), "PMT ", ipmt + 1);
    H1_adctdc_diff_time[ipmt]->SetTitle(newTitle);
    H1_adctdc_diff_time[ipmt]->GetXaxis()->SetTitle("ADC-TDC time difference (ns)");
    H1_adctdc_diff_time[ipmt]->GetYaxis()->SetTitle("Counts / bin");
    H1_adctdc_diff_time[ipmt]->GetXaxis()->CenterTitle();
    H1_adctdc_diff_time[ipmt]->GetYaxis()->CenterTitle();
    H1_adctdc_diff_time[ipmt]->GetXaxis()->SetTitleSize(0.045);
    H1_adctdc_diff_time[ipmt]->GetYaxis()->SetTitleSize(0.045);
    H1_adctdc_diff_time[ipmt]->GetXaxis()->SetLabelSize(0.04);
    H1_adctdc_diff_time[ipmt]->GetYaxis()->SetLabelSize(0.04);
    H1_adctdc_diff_time[ipmt]->GetXaxis()->SetTitleOffset(1.65);
    H1_adctdc_diff_time[ipmt]->GetYaxis()->SetTitleOffset(1.65);
    ymax = H1_adctdc_diff_time[ipmt]->GetBinContent(H1_adctdc_diff_time[ipmt]->GetMaximumBin());
    if (histname.Contains("hodo")) {
      minVal = minArr[ipmt * 4 + offset];
      maxVal = maxArr[ipmt * 4 + offset];
    } else if (histname.Contains("cal_h")) {
      minVal = minArr[ipmt + offset];
      maxVal = maxArr[ipmt + offset];
    } else {
      minVal = minArr[ipmt];
      maxVal = maxArr[ipmt];
    }

    // Fixed display ranges for each detector family.
    double displayMin = ladplot::number("CER_X_MIN", -800.);
    double displayMax = ladplot::number("CER_X_MAX", 800.);
    if (histname.Contains("hodo")) {
      displayMin = ladplot::number("HODO_X_MIN", -250.);
      displayMax = ladplot::number("HODO_X_MAX", 150.);
    } else if (histname.Contains("cal")) {
      displayMin = ladplot::number("CAL_X_MIN", -500.);
      displayMax = ladplot::number("CAL_X_MAX", 500.);
    }
    H1_adctdc_diff_time[ipmt]->GetXaxis()->SetRangeUser(displayMin, displayMax);
    H1_adctdc_diff_time[ipmt]->SetMaximum(ymax > 0 ? ymax * 3. : 1.);
    H1_adctdc_diff_time[ipmt]->Draw("HIST");
    if (ymax > 0) {
      const double peakX =
          H1_adctdc_diff_time_peak[ipmt] != 0
              ? H1_adctdc_diff_time_peak[ipmt]
              : H1_adctdc_diff_time[ipmt]->GetBinCenter(H1_adctdc_diff_time[ipmt]->GetMaximumBin());
      TMarker *peakMarker = new TMarker(peakX, ymax * 1.35, 23);
      peakMarker->SetMarkerColor(kRed);
      peakMarker->SetMarkerSize(1.3);
      peakMarker->Draw();
    }
    currentCanvas->Update();

    minLine = new TLine(minVal, 0, minVal, ymax);
    maxLine = new TLine(maxVal, 0, maxVal, ymax);
    if (H1_adctdc_diff_time_peak[ipmt] > minVal && H1_adctdc_diff_time_peak[ipmt] < maxVal) {
      minLine->SetLineColor(kGreen);
      maxLine->SetLineColor(kGreen);
    } else {
      minLine->SetLineColor(kRed);
      maxLine->SetLineColor(kRed);
    }
    minLine->Draw();
    maxLine->Draw();
    H1_adctdc_diff_time[ipmt]->SetStats(0);
    gPad->SetLogy();
    currentCanvas->Update();
  }
  if (newWindows == true) {
    TString bound = "";
    // hodo_PosAdcTimeWindowMin
    double halfWindow = -1 * width / 2.;
    for (int i = 0; i < 2; i++) {
      if (i == 0)
        bound = "Min";
      if (i == 1)
        bound = "Max";
      TString side = "";
      if (polarity == 1)
        side = "Pos";
      if (polarity == 2)
        side = "Neg";
      TString DetectorPeakName = spect + detector + "_" + side + "AdcTimeWindow" + bound + "= ";
      if (detector == "cal_prshwr") {
        DetectorPeakName = spect + "cal_" + side + "AdcTimeWindow" + bound + "= ";
      }
      if (detector == "cal_shwr") {
        DetectorPeakName = spect + "cal_arr_" + side + "AdcTimeWindow" + bound + "= ";
      }
      if (detector == "cal_hA") {
        DetectorPeakName = spect + "cal_" + side + "AdcTimeWindow" + bound + "= ";
      }
      if (detector == "cal_hB" || detector == "cal_hC" || detector == "cal_hD") {
      } else {
        cout << DetectorPeakName;
      }
      for (Int_t ipmt = 0; ipmt < (H1_pmt->GetSize() - 2); ipmt++) {
        cout << H1_adctdc_diff_time_peak[ipmt] + halfWindow;
        if (ipmt < (H1_pmt->GetSize() - 3))
          cout << ", ";
        if (ipmt != 0 && ipmt % 16 == 0)
          cout << endl;
      }
      cout << endl;
      if (detector == "cal_hB" && polarity == 2) {
        for (Int_t ipmt = 0; ipmt < (H1_pmt->GetSize() - 2); ipmt++) {
          cout << 0;
          if (ipmt < (H1_pmt->GetSize() - 3))
            cout << ", ";
          if (ipmt != 0 && ipmt % 16 == 0)
            cout << endl;
        }
        cout << endl;
        for (Int_t ipmt = 0; ipmt < (H1_pmt->GetSize() - 2); ipmt++) {
          cout << 0;
          if (ipmt < (H1_pmt->GetSize() - 3))
            cout << ", ";
          if (ipmt != 0 && ipmt % 16 == 0)
            cout << endl;
        }
        cout << endl;
      }
      halfWindow = -1 * halfWindow;
    }
  }
  f2->cd();
  save_timing_window_views(currentCanvas, f2);
  f2->Close();
}

void export_timing_window_directory(TString root, TString pdf, TString directory) {
  ladplot::exportPDF(root, pdf, ladplot::canvases(root, directory));
}
void export_timing_windows_pdf(TString root, TString pdf) {
  export_timing_window_directory(root, pdf, "original");
  if (ladplot::flag("DETAILED", true)) {
    TString detail = pdf;
    detail.ReplaceAll(".pdf", "_detailed.pdf");
    export_timing_window_directory(root, detail, "detailed");
  }
}
