#include "../../plot_common.h"
#include <TChain.h>
#include <TFile.h>
#include <TTree.h>
#include <TLeaf.h>
#include <TCanvas.h>
#include <TKey.h>
#include <TH1D.h>
#include <TLatex.h>
#include <TLine.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TROOT.h>
#include <THcGlobals.h>
#include <THcParmList.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <iomanip>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
void plot_trig_timing_windows_lad(const std::vector<TString> &inputs, TString arm, int run,
                                  TString root) {
  try {
    const TString repo = ladplot::setting("PLOT_REPO");
    if (arm != "hms" && arm != "shms")
      throw std::runtime_error("Group must be hms or shms");
    gROOT->SetBatch(true);
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);
    TChain source("trigger_plot_input");
    for (auto path : inputs) {
      TFile f(path, "READ");
      if (f.IsZombie() || !f.Get("T"))
        throw std::runtime_error("Missing input T");
      source.AddFile(path, TTree::kMaxEntries, "T");
    }
    source.LoadTree(0);
    auto tree = &source;
    int countADC = 0, countTDC = 0;
    std::string namesADC, namesTDC;
    TString prefix = "t_" + arm, param = ladplot::setting(("TRIG_PARAM_" + arm).Data(),
                                                          ("PARAM/TRIG/t" + arm + ".param").Data());
    ladplot::parameters(run);
    gHcParms->Load(param);
    ladplot::overrideParameters();
    DBRequest config[] = {{"_numAdc", &countADC, kInt},
                          {"_numTdc", &countTDC, kInt},
                          {"_adcNames", &namesADC, kString},
                          {"_tdcNames", &namesTDC, kString},
                          {0}};
    gHcParms->LoadParmValues(config, prefix.Data());
    std::vector<std::vector<std::string>> names(2);
    for (int type = 0; type < 2; ++type) {
      std::istringstream words(type ? namesTDC : namesADC);
      std::string x;
      while (words >> x)
        names[type].push_back(x);
      if (names[type].size() != (size_t)(type ? countTDC : countADC))
        throw std::runtime_error("Parameter channel count mismatch");
    }
    // Require real ADC/TDC branches before creating any output.
    for (int type = 0; type < 2; ++type)
      for (auto name : names[type]) {
        TString stem = "T." + arm + "." + name.c_str();
        for (auto suffix : {TString(type ? "_tdcTimeRaw" : "_adcPulseTimeRaw"),
                            TString(type ? "_tdcMultiplicity" : "_adcMultiplicity")})
          if (!tree->GetLeaf(stem + suffix))
            throw std::runtime_error(std::string("Missing input branch ") + (stem + suffix).Data());
      }
    TString upper = arm;
    upper.ToUpper();
    TString pending = root + ".partial";
    TFile output(pending, "RECREATE");
    std::vector<TString> pages;
    int total = 0;
    tree->SetEstimate(tree->GetEntries() + 1);
    for (int type = 0; type < 2; ++type) {
      int n = names[type].size();
      std::vector<double> lo(n, 0), hi(n, 100000);
      TString kind = type ? "Tdc" : "Adc";
      TString minKey = "_" + kind + "TimeWindowMin", maxKey = "_" + kind + "TimeWindowMax";
      DBRequest windows[] = {{minKey.Data(), lo.data(), kDouble, (UInt_t)n, 1},
                             {maxKey.Data(), hi.data(), kDouble, (UInt_t)n, 1},
                             {0}};
      gHcParms->LoadParmValues(windows, prefix.Data());
      auto group = output.mkdir(type ? "TDC" : "ADC"), histDir = group->mkdir("histograms"),
           canvasDir = group->mkdir("canvases");
      TCanvas *canvas = nullptr;
      for (int index = 0; index < n; ++index) {
        if (index % 16 == 0) {
          canvas = new TCanvas(
              Form("%s_%s_%02d", arm.Data(), type ? "TDC" : "ADC", index / 16 + 1),
              Form("%s %s channels; run %d", upper.Data(), type ? "TDC" : "ADC", run), 1600, 1600);
          canvas->SetCanvasSize(1600, 1600);
          canvas->Divide(4, 4, .006, .006);
        }
        TString stem = "T." + arm + "." + names[type][index].c_str(),
                variable = stem + (type ? "_tdcTimeRaw" : "_adcPulseTimeRaw"),
                mult = stem + (type ? "_tdcMultiplicity" : "_adcMultiplicity");
        Long64_t hits = tree->Draw(variable, mult + ">0", "goff");
        if (hits < 0)
          throw std::runtime_error("Invalid branch expression");
        double a = 1e99, b = -1e99;
        for (Long64_t k = 0; k < hits; ++k) {
          double x = tree->GetV1()[k];
          if (!std::isfinite(x))
            throw std::runtime_error("Non-finite raw time");
          a = std::min(a, x);
          b = std::max(b, x);
        }
        double width = ladplot::number(type ? "TDC_BIN_WIDTH" : "ADC_BIN_WIDTH", type ? 5. : 2.),
               left = lo[index], right = hi[index];
        // No visual padding: use the selected data extrema. ROOT excludes the upper
        // histogram edge, so advance that edge a few floating-point steps to retain max.
        if (hits) {
          left = a;
          right = b;
          for (int edgeStep = 0; edgeStep < 16; ++edgeStep)
            right = std::nextafter(right, std::numeric_limits<double>::infinity());
          if (a == b) {
            left = a - width / 2.;
            right = a + width / 2.;
          }
        }
        if (right <= left)
          right = left + width;
        int bins = std::max(1, (int)std::ceil((right - left) / width));
        width = (right - left) / bins;
        auto h =
            new TH1D(Form("%s_%03d", type ? "TDC" : "ADC", index + 1),
                     variable + Form(";Raw %s time (channel);Counts / bin", type ? "TDC" : "ADC"),
                     bins, left, right);
        h->SetDirectory(nullptr);
        for (Long64_t k = 0; k < hits; ++k)
          h->Fill(tree->GetV1()[k]);
        if (h->GetBinContent(0) || h->GetBinContent(bins + 1))
          throw std::runtime_error(std::string("Visible raw-time range loses hits: ") +
                                   variable.Data());
        auto pad = canvas->cd(index % 16 + 1);
        pad->SetLeftMargin(.20);
        pad->SetRightMargin(.07);
        pad->SetBottomMargin(.20);
        pad->SetTopMargin(.22);
        pad->SetLogy();
        h->SetStats(false);
        h->SetLineColor(kBlue);
        h->SetLineWidth(1);
        h->SetMinimum(.5);
        h->SetMaximum(std::max(2., h->GetMaximum() * 2.));
        for (auto axis : {h->GetXaxis(), h->GetYaxis()}) {
          axis->SetTitleSize(.041);
          axis->SetLabelSize(.037);
          axis->CenterTitle();
        }
        h->GetXaxis()->SetTitleOffset(1.35);
        h->GetYaxis()->SetTitleOffset(1.7);
        h->GetXaxis()->SetNdivisions(505);
        h->GetXaxis()->SetNoExponent(true);
        h->Draw("HIST");
        auto text = [&](double y, TString value, int color) {
          auto l = new TLatex(.20, y, value);
          l->SetNDC();
          l->SetTextSize(.030);
          l->SetTextColor(color);
          l->Draw();
        };
        text(.965, stem, kBlack);
        text(.915, Form("%s raw; mult > 0; N = %lld", type ? "TDC" : "ADC", hits), kBlue);
        text(.853, Form("window [%.0f, %.0f] ch", lo[index], hi[index]), kRed);
        for (double bound : {lo[index], hi[index]})
          if (bound >= left && bound <= right) {
            auto line = new TLine(bound, .5, bound, h->GetMaximum());
            line->SetLineColor(kRed);
            line->SetLineStyle(2);
            line->SetLineWidth(1);
            line->Draw();
          }
        if (!hits) {
          text(.65, "No selected hits", kBlack);
        }
        histDir->cd();
        h->Write();
        ++total;
        if (index % 16 == 15 || index == n - 1) {
          canvas->Modified();
          canvas->Update();
          canvasDir->cd();
          canvas->Write();
          pages.push_back(TString(type ? "TDC" : "ADC") + "/canvases/" + canvas->GetName());
          delete canvas;
          canvas = nullptr;
        }
      }
    }
    output.Close();
    TFile saved(pending, "READ");
    int checked = 0;
    for (const auto &path : pages) {
      auto c = dynamic_cast<TCanvas *>(saved.Get(path));
      if (!c)
        throw std::runtime_error("Missing saved TRIG page");
      for (int p = 1; p <= 16; ++p) {
        auto pad = c->GetPad(p);
        TIter objects(pad->GetListOfPrimitives());
        while (auto o = objects())
          if (auto h = dynamic_cast<TH1 *>(o)) {
            ++checked;
            if (TString(h->GetXaxis()->GetTitle()).IsNull() ||
                TString(h->GetYaxis()->GetTitle()).IsNull())
              throw std::runtime_error("Missing TRIG axes");
          }
      }
      delete c;
    }
    saved.Close();
    if (checked != total)
      throw std::runtime_error("Missing TRIG channel");
    if (gSystem->Rename(pending, root))
      throw std::runtime_error("Cannot publish TRIG ROOT");
    TString pdf = root;
    pdf.ReplaceAll(".root", ".pdf");
    ladplot::exportPDF(root, pdf, pages);
    std::cout << "TRIG channels " << total << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "TRIG_ERROR " << e.what() << std::endl;
    gSystem->Exit(1);
  }
}
