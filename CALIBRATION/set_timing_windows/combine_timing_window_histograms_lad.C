#include "TAxis.h"
#include "TFile.h"
#include "TH2F.h"
#include "TString.h"
#include "TSystem.h"

#include <cmath>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <cstdlib>

using namespace std;

vector<TString> hms_timing_window_histogram_names_lad() {
  return {
      "hhodo_1x_good_adctdc_diff_time_vs_pmt_pos", "hhodo_1x_good_adctdc_diff_time_vs_pmt_neg",
      "hhodo_1y_good_adctdc_diff_time_vs_pmt_pos", "hhodo_1y_good_adctdc_diff_time_vs_pmt_neg",
      "hhodo_2x_good_adctdc_diff_time_vs_pmt_pos", "hhodo_2x_good_adctdc_diff_time_vs_pmt_neg",
      "hhodo_2y_good_adctdc_diff_time_vs_pmt_pos", "hhodo_2y_good_adctdc_diff_time_vs_pmt_neg",
      "hcer_good_adctdc_diff_time_vs_pmt",         "hcal_hA_good_adctdc_diff_time_vs_pmt_pos",
      "hcal_hB_good_adctdc_diff_time_vs_pmt_pos",  "hcal_hC_good_adctdc_diff_time_vs_pmt_pos",
      "hcal_hD_good_adctdc_diff_time_vs_pmt_pos",  "hcal_hA_good_adctdc_diff_time_vs_pmt_neg",
      "hcal_hB_good_adctdc_diff_time_vs_pmt_neg",
  };
}

vector<TString> shms_timing_window_histogram_names_lad() {
  return {
      "phodo_1x_good_adctdc_diff_time_vs_pmt_pos",
      "phodo_1x_good_adctdc_diff_time_vs_pmt_neg",
      "phodo_1y_good_adctdc_diff_time_vs_pmt_pos",
      "phodo_1y_good_adctdc_diff_time_vs_pmt_neg",
      "phodo_2x_good_adctdc_diff_time_vs_pmt_pos",
      "phodo_2x_good_adctdc_diff_time_vs_pmt_neg",
      "phodo_2y_good_adctdc_diff_time_vs_pmt_pos",
      "phodo_2y_good_adctdc_diff_time_vs_pmt_neg",
      "pcal_prshwr_good_adctdc_diff_time_vs_pmt_pos",
      "pcal_prshwr_good_adctdc_diff_time_vs_pmt_neg",
      "pcal_shwr_good_adctdc_diff_time_vs_pmt",
      "phgcer_good_adctdc_diff_time_vs_pmt",
      "pngcer_good_adctdc_diff_time_vs_pmt",
  };
}

bool compatible_axis_timing_lad(const TAxis *left, const TAxis *right) {
  if (!left || !right || left->GetNbins() != right->GetNbins())
    return false;

  const double tolerance = 1e-9;
  for (int bin = 1; bin <= left->GetNbins() + 1; ++bin) {
    if (fabs(left->GetBinLowEdge(bin) - right->GetBinLowEdge(bin)) > tolerance)
      return false;
  }
  return true;
}

bool compatible_binning_timing_lad(const TH2F *left, const TH2F *right) {
  return left && right && compatible_axis_timing_lad(left->GetXaxis(), right->GetXaxis()) &&
         compatible_axis_timing_lad(left->GetYaxis(), right->GetYaxis());
}

bool combine_timing_window_histograms_lad(const vector<TString> &inputFiles,
                                          const vector<TString> &histogramNames,
                                          const TString &outputFile) {
  if (inputFiles.empty()) {
    cerr << "No input ROOT files were supplied" << endl;
    return false;
  }

  TFile output(outputFile, "RECREATE");
  if (output.IsZombie()) {
    cerr << "Cannot create " << outputFile << endl;
    return false;
  }

  for (const TString &histogramName : histogramNames) {
    TH2F *sum = nullptr;
    for (const TString &inputFile : inputFiles) {
      TFile input(inputFile, "READ");
      if (input.IsZombie()) {
        cerr << "Cannot open " << inputFile << endl;
        delete sum;
        output.Close();
        return false;
      }

      TH2F *source = dynamic_cast<TH2F *>(input.Get(histogramName));
      if (!source) {
        cerr << "Missing " << histogramName << " in " << inputFile << endl;
        delete sum;
        output.Close();
        return false;
      }

      if (!sum) {
        sum = dynamic_cast<TH2F *>(source->Clone(histogramName));
        sum->SetDirectory(nullptr);
        sum->Sumw2(kTRUE);
      } else {
        if (!compatible_binning_timing_lad(sum, source)) {
          cerr << "Incompatible binning for " << histogramName << " in " << inputFile << endl;
          delete sum;
          output.Close();
          return false;
        }
        sum->Add(source);
      }
    }

    output.cd();
    sum->Write(histogramName);
    delete sum;
  }

  output.Close();
  return true;
}
