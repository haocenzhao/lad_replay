#include "../../timing_window_setup_lad.C"
#include <TTree.h>
#include <TH2F.h>
#include <THcGlobals.h>
#include <THcParmList.h>
#include <stdexcept>
namespace aero16 {
TString key(int side) {
  return TString("paero_good_adctdc_diff_time_vs_pmt_") + (side ? "neg" : "pos");
}
} // namespace aero16
void build_aero_histograms(TString input, TString output) {
  using namespace aero16;
  TFile f(input, "READ");
  auto t = dynamic_cast<TTree *>(f.Get("T"));
  if (f.IsZombie() || f.TestBit(TFile::kRecovered) || !t)
    throw std::runtime_error("Bad AERO input");
  TFile result(output, "RECREATE");
  for (int side = 0; side < 2; ++side) {
    TString cap = side ? "Neg" : "Pos", value = "P.aero.good" + cap + "AdcTdcDiffTime",
            mult = "P.aero.good" + cap + "AdcMult";
    if (!t->GetLeaf(value) || !t->GetLeaf(mult))
      throw std::runtime_error("Missing AERO branches");
    auto h = new TH2F(key(side), "SHMS AERO " + cap + " Good AdcTdc Diff Time vs. PMT Number", 7,
                      .5, 7.5, 1000, -1000, 1000);
    t->Draw(value + ":Iteration$+1>>" + key(side), mult + ">0&&abs(" + value + ")<1000", "goff");
    // Independent event/PMT scan checks the ROOT array-expression selection.
    t->SetEstimate(7 * t->GetEntries() + 1);
    Long64_t n = t->Draw(value + ":" + mult, "", "goff");
    if (n != 7 * t->GetEntries())
      throw std::runtime_error("Unexpected AERO PMT array size");
    TH2F check("independent_aero_check", "", 7, .5, 7.5, 1000, -1000, 1000);
    check.SetDirectory(nullptr);
    for (Long64_t i = 0; i < n; ++i) {
      double x = t->GetV1()[i], m = t->GetV2()[i];
      if (m > 0 && std::isfinite(x) && std::abs(x) < 1000)
        check.Fill(i % 7 + 1, x);
    }
    if (h->GetEntries() != check.GetEntries())
      throw std::runtime_error("AERO independent count mismatch");
    for (int i = 0; i < h->GetNcells(); ++i)
      if (h->GetBinContent(i) != check.GetBinContent(i))
        throw std::runtime_error("AERO independent bin mismatch");
    result.cd();
    h->Write();
    delete h;
  }
}
void sum_aero_histograms(std::vector<TString> inputs, TString output) {
  using namespace aero16;
  TFile result(output, "RECREATE");
  for (int side = 0; side < 2; ++side) {
    TH2F *sum = nullptr;
    for (auto path : inputs) {
      TFile f(path, "READ");
      auto h = dynamic_cast<TH2F *>(f.Get(key(side)));
      if (!h)
        throw std::runtime_error("Missing combined AERO source");
      if (!sum) {
        sum = (TH2F *)h->Clone();
        sum->SetDirectory(nullptr);
      } else
        sum->Add(h);
    }
    result.cd();
    sum->Write(key(side));
    delete sum;
  }
}

void run_aero_plots_lad(const std::vector<TString> &inputs, int parameterRun, TString output,
                        TString label) {
  ladplot::parameters(parameterRun);
  auto parameter = ladplot::setting("AERO_PARAM_FILE", "PARAM/SHMS/AERO/paero_cuts.param");
  gHcParms->Load(parameter);
  ladplot::overrideParameters();
  double lo[2][7] = {}, hi[2][7] = {};
  DBRequest req[] = {{"aero_adcPosTimeWindowMin", lo[0], kDouble, 7, 1},
                     {"aero_adcPosTimeWindowMax", hi[0], kDouble, 7, 1},
                     {"aero_adcNegTimeWindowMin", lo[1], kDouble, 7, 1},
                     {"aero_adcNegTimeWindowMax", hi[1], kDouble, 7, 1},
                     {0}};
  gHcParms->LoadParmValues(req, "p");
  std::vector<TString> histograms;
  for (size_t i = 0; i < inputs.size(); ++i) {
    TString file = ladplot::setting("PLOT_SCRATCH") + Form("/aero_%zu.root", i);
    build_aero_histograms(inputs[i], file);
    histograms.push_back(file);
  }
  TString total = ladplot::setting("PLOT_SCRATCH") + "/aero_sum.root";
  sum_aero_histograms(histograms, total);
  {
    TFile fresh(output, "RECREATE");
  }
  for (int side = 0; side < 2; ++side)
    calc_timing_windows(total, output, "aero", "p", side + 1, lo[side], hi[side], false, 40.);
  {
    TFile saved(output, "UPDATE");
    auto d = saved.GetDirectory("original");
    for (int side = 0; side < 2; ++side) {
      auto c =
          dynamic_cast<TCanvas *>(d->Get(side ? "paero_neg_diff_time_1" : "paero_pos_diff_time_1"));
      if (!c)
        throw std::runtime_error("Missing AERO canvas");
      c->SetCanvasSize(1600, 1600);
      for (int p = 1; p <= 7; ++p) {
        auto pad = c->GetPad(p);
        pad->SetLeftMargin(.22);
        pad->SetRightMargin(.06);
        pad->SetBottomMargin(.20);
        pad->SetTopMargin(.12);
        TIter objects(pad->GetListOfPrimitives());
        while (auto o = objects())
          if (auto h = dynamic_cast<TH1 *>(o)) {
            h->SetTitle(Form("SHMS AERO %s PMT %d, %s", side ? "Neg" : "Pos", p, label.Data()));
            h->GetXaxis()->SetRangeUser(ladplot::number("AERO_X_MIN", -450.),
                                        ladplot::number("AERO_X_MAX", 450.));
            h->GetXaxis()->SetTitleOffset(1.35);
            h->GetYaxis()->SetTitleOffset(1.30);
            h->SetLineColor(kBlue);
            h->SetMinimum(.5);
          }
        pad->Modified();
        pad->Update();
      }
      c->Modified();
      c->Update();
      d->cd();
      c->Write(c->GetName(), TObject::kOverwrite);
      delete c;
    }
  }
  TString pdf = output;
  pdf.ReplaceAll(".root", ".pdf");
  ladplot::exportPDF(output, pdf, ladplot::canvases(output, "original"));
}
