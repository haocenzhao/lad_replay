#include "../../plot_common.h"
#include <TTree.h>
#include <TH1D.h>
#include <TLine.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TNamed.h>
#include <TParameter.h>
#include <TVectorD.h>
#include <TGaxis.h>
#include <memory>
#include <algorithm>
namespace dc16 {
struct Spectra {
  Long64_t events = 0;
  std::vector<std::unique_ptr<TH1D>> hist;
};
inline std::vector<std::string> planes(TString arm) {
  return arm == "HMS" ? std::vector<std::string>{"1u1", "1u2", "1x1", "1x2", "1v2", "1v1",
                                                 "2v1", "2v2", "2x2", "2x1", "2u2", "2u1"}
                      : std::vector<std::string>{"1u1", "1u2", "1x1", "1x2", "1v1", "1v2",
                                                 "2v2", "2v1", "2x2", "2x1", "2u2", "2u1"};
}
Spectra loadInputs(const std::vector<TString> &files) {
  Spectra sum;
  for (auto path : files) {
    TFile f(path);
    auto t = dynamic_cast<TTree *>(f.Get("T"));
    if (f.IsZombie() || !t)
      throw std::runtime_error("Missing DC input T");
    sum.events += t->GetEntries();
    int index = 0;
    for (auto arm : {"HMS", "SHMS"})
      for (auto plane : planes(arm)) {
        TString leaf = Form("%s.dc.%s.rawtdc", TString(arm) == "HMS" ? "H" : "P", plane.c_str());
        if (!t->GetLeaf(leaf))
          throw std::runtime_error("Missing DC plane " + std::string(leaf.Data()));
        double low = ladplot::number("DC_STORAGE_MIN", -30000.),
               high = ladplot::number("DC_STORAGE_MAX", 30000.),
               width = ladplot::number("DC_BIN_WIDTH", 5.);
        TH1D h(Form("dc_source_%d", index), "", int((high - low) / width), low, high);
        h.SetDirectory(nullptr);
        t->SetEstimate(t->GetEntries() + 1);
        auto n = t->Draw(leaf, "", "goff");
        if (n > t->GetEstimate()) {
          t->SetEstimate(n + 1);
          n = t->Draw(leaf, "", "goff");
        }
        if (n < 0)
          throw std::runtime_error("DC Draw failed");
        for (Long64_t i = 0; i < n; ++i)
          h.Fill(t->GetV1()[i]);
        if (sum.hist.size() <= size_t(index)) {
          auto c = std::unique_ptr<TH1D>((TH1D *)h.Clone());
          c->SetDirectory(nullptr);
          sum.hist.push_back(std::move(c));
        } else if (!sum.hist[index]->Add(&h))
          throw std::runtime_error("Incompatible DC bins");
        ++index;
      }
  }
  return sum;
}
inline void axisStyle(TAxis *axis) {
  axis->SetAxisColor(kBlack);
  axis->SetLabelColor(kBlack);
  axis->SetTitleColor(kBlack);
  axis->SetLabelFont(42);
  axis->SetTitleFont(42);
  axis->SetLabelSize(.037);
  axis->SetTitleSize(.043);
  axis->SetTickLength(.03);
  axis->SetLabelOffset(.010);
  axis->SetNoExponent(kTRUE);
}
void drawSample(TFile &output, const Spectra &nt, const Spectra &prod, const TString &label,
                bool comparison) {
  if (nt.events != prod.events)
    throw std::runtime_error("Event totals differ");
  auto sample = output.mkdir(label);
  sample->cd();
  TParameter<Long64_t>("events", nt.events).Write();
  for (int ia = 0; ia < 2; ia++) {
    const TString arm = ia ? "SHMS" : "HMS";
    const char *prefix = ia ? "p" : "h";
    Int_t minimum[12], maximum[12], tzero = 0;
    DBRequest request[] = {{"dc_tdc_min_win", minimum, kInt, 12, 1},
                           {"dc_tdc_max_win", maximum, kInt, 12, 1},
                           {"_using_tzero_per_wire", &tzero, kInt, 0, 1},
                           {0}};
    gHcParms->LoadParmValues(request, prefix);
    if (tzero != 1)
      throw std::runtime_error("Per-wire tzero must remain enabled");
    auto armDirectory = sample->mkdir(arm);
    auto names = planes(arm);
    for (int view = 0; view < (ladplot::flag("DETAILED", true) ? 2 : 1); view++) {
      auto directory = armDirectory->mkdir(view ? "detailed" : "original");
      directory->cd();
      auto canvas = new TCanvas(Form("%s_%s_%s_%d", comparison ? "comparison" : "noTiming",
                                     label.Data(), arm.Data(), view),
                                "DC timing windows", 1900, 1450);
      canvas->SetCanvasSize(1900, 1450);
      canvas->Divide(4, 3, .006, .012);
      for (int ip = 0; ip < 12; ip++) {
        const auto &plane = names[ip];
        const int index = ia * 12 + ip;
        auto noTiming =
            static_cast<TH1D *>(nt.hist[index]->Clone(Form("noTiming_%s", plane.c_str())));
        noTiming->SetDirectory(nullptr);
        auto production =
            comparison
                ? static_cast<TH1D *>(prod.hist[index]->Clone(Form("production_%s", plane.c_str())))
                : nullptr;
        if (production)
          production->SetDirectory(nullptr);
        TString sampleLabel = ladplot::setting("PLOT_LABEL", label.Data());
        TString title = Form("%s DC %s, %s;Selected-hit raw TDC (channel);Selected hits / bin",
                             arm.Data(), plane.c_str(), sampleLabel.Data());
        const double margin = ladplot::number("DC_DETAIL_MARGIN", 600.);
        const double low = view ? minimum[ip] - margin : ladplot::number("DC_X_MIN", -16000.),
                     high = view ? maximum[ip] + margin : ladplot::number("DC_X_MAX", -2000.);
        for (auto histogram : {noTiming, production})
          if (histogram) {
            histogram->SetTitle(title);
            histogram->SetStats(0);
            histogram->SetOption("HIST");
            histogram->GetXaxis()->SetRangeUser(low, high);
            axisStyle(histogram->GetXaxis());
            axisStyle(histogram->GetYaxis());
            histogram->GetXaxis()->SetNdivisions(505);
            histogram->GetYaxis()->SetTitleOffset(1.6);
            histogram->SetBit(TObject::kCanDelete);
          }
        noTiming->SetLineColor(comparison ? kOrange + 7 : kBlue);
        noTiming->SetLineStyle(comparison ? 7 : 1);
        noTiming->SetLineWidth(comparison ? 3 : 1);
        if (production) {
          production->SetLineColor(kBlue);
          production->SetLineStyle(1);
          production->SetLineWidth(1);
        }
        double top = std::max(1., noTiming->GetMaximum());
        if (production)
          top = std::max(top, production->GetMaximum());
        const double ymax = top * (comparison ? 20 : 3);
        auto pad = canvas->cd(ip + 1);
        pad->SetLogx(0);
        pad->SetLogy();
        pad->SetTicks(1, 1);
        pad->SetLeftMargin(.20);
        pad->SetRightMargin(.08);
        pad->SetBottomMargin(.20);
        pad->SetTopMargin(.12);
        // The axis owner is independent of both curves and is never redrawn with SAME.
        auto axes = pad->DrawFrame(low, .5, high, ymax, title);
        axes->SetName(Form("axes_%s", plane.c_str()));
        axes->SetDirectory(nullptr);
        axes->SetStats(0);
        axisStyle(axes->GetXaxis());
        axisStyle(axes->GetYaxis());
        // Exact major divisions include both endpoints, also when ROOT reopens the canvas.
        axes->GetXaxis()->SetNdivisions(view ? 506 : 507, kFALSE);
        axes->GetXaxis()->SetLabelSize(.031);
        axes->GetXaxis()->SetTitleOffset(1.35);
        axes->GetYaxis()->SetNoExponent(kFALSE);
        axes->GetYaxis()->SetLabelSize(.033);
        axes->GetYaxis()->SetTitleSize(.040);
        axes->GetYaxis()->SetTitleOffset(2.0);
        axes->SetBit(TObject::kCanDelete);
        noTiming->Draw("HIST SAME");
        if (production)
          production->Draw("HIST SAME");
        for (int bound : {minimum[ip], maximum[ip]}) {
          auto line = new TLine(bound, .5, bound, top * 1.5);
          line->SetLineColor(kRed);
          line->SetLineStyle(2);
          line->SetLineWidth(1);
          line->SetBit(TObject::kCanDelete);
          line->Draw();
        }
        if (comparison) {
          auto legend = new TLegend(.48, .785, .90, .865);
          legend->SetBorderSize(0);
          legend->SetFillStyle(0);
          legend->SetTextSize(.031);
          legend->SetBit(TObject::kCanDelete);
          legend->AddEntry(production, "Spectrometers only", "l");
          legend->AddEntry(noTiming, "No timing windows", "l");
          legend->Draw();
        }
        auto cut = new TLatex(.18, .900, Form("cut = [%d, %d]", minimum[ip], maximum[ip]));
        cut->SetNDC();
        cut->SetTextFont(62);
        cut->SetTextSize(.041);
        cut->SetTextColor(kRed);
        cut->SetBit(TObject::kCanDelete);
        cut->Draw();
        if (noTiming->GetEntries() == 0 && (!production || production->GetEntries() == 0)) {
          auto empty = new TLatex(.23, .55, "No selected hits");
          empty->SetNDC();
          empty->SetTextSize(.05);
          empty->SetBit(TObject::kCanDelete);
          empty->Draw();
        }
        pad->Modified();
        pad->Update();
      }
      canvas->cd();
      canvas->SetCanvasSize(1900, 1450);
      canvas->Modified();
      canvas->Update();
      directory->cd();
      canvas->Write("canvas");
      delete canvas;
    }
  }
  std::cout << "STAGED_DC_SAMPLE " << (comparison ? "comparison" : "no_timing") << " " << label
            << std::endl;
}
} // namespace dc16

void run_dc_plots_lad(const std::vector<TString> &noTiming, const std::vector<TString> &production,
                      int run, TString outputBase, TString sample) {
  gROOT->SetBatch(true);
  ladplot::parameters(run);
  auto nt = dc16::loadInputs(noTiming);
  TString mode = ladplot::setting("DC_MODE", "both");
  dc16::Spectra prod;
  if (mode != "no_timing")
    prod = dc16::loadInputs(production);
  for (bool comparison : {false, true}) {
    if ((mode == "no_timing" && comparison) || (mode == "comparison" && !comparison))
      continue;
    TString root = outputBase + (comparison ? "DC_comparison_" : "DC_noTiming_") + sample + ".root";
    {
      TFile out(root, "RECREATE");
      dc16::drawSample(out, nt, comparison ? prod : nt, sample, comparison);
    }
    std::vector<TString> pages;
    for (auto arm : {"HMS", "SHMS"})
      for (int view = 0; view < (ladplot::flag("DETAILED", true) ? 2 : 1); ++view)
        pages.push_back(sample + "/" + arm + "/" + (view ? "detailed" : "original") + "/canvas");
    TString pdf = root;
    pdf.ReplaceAll(".root", ".pdf");
    ladplot::exportPDF(root, pdf, pages);
  }
}
