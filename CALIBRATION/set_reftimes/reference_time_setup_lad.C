#include "../set_timing_windows/plot_common.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TH1F.h"
#include "TLegend.h"
#include "TLine.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TString.h"
#include "THcGlobals.h"
#include "THcParmList.h"
#include "TTree.h"
#include "TLeaf.h"
#include "TBranch.h"
#include "TDirectory.h"
#include "TParameter.h"
#include "TNamed.h"
#include <map>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

using namespace std;

struct ReferenceEventDataLad {
  TH1F *multiplicity = nullptr;
  TH1F *all = nullptr;
  TH1F *mult[3] = {nullptr, nullptr, nullptr};
  TH1F *edtmTime = nullptr;
  TH1F *edtmMultiplicity = nullptr;
  TLeaf *timeLeaf = nullptr;
  TLeaf *multLeaf = nullptr;
  TString timeVariable, multVariable, edtmDefinition;
  bool combined = false;
  int combinedRuns = 1;
  TString combinedRunList;
  Long64_t events = 0, edtmHits = 0, nonzero = 0, zeroTime = 0;
};

using ReferenceEventMapLad = map<TString, ReferenceEventDataLad>;

void style_edtm_reference_lad(TH1F *histogram) {
  histogram->SetStats(0);
  histogram->SetLineColor(kGreen + 2);
  histogram->SetLineStyle(2);
  histogram->SetLineWidth(2);
  histogram->SetFillStyle(0);
}

void write_reference_event_data_lad(TFile *output, ReferenceEventMapLad &data) {
  if (!data.empty() && data.begin()->second.combined) {
    output->cd();
    TParameter<int>("combined_run_count", data.begin()->second.combinedRuns).Write();
    TNamed("combined_runs", data.begin()->second.combinedRunList.Data()).Write();
  }
  auto folder = output->GetDirectory("histograms");
  if (!folder)
    folder = output->mkdir("histograms");
  for (auto &item : data) {
    auto &entry = item.second;
    auto directory = folder->GetDirectory(item.first);
    if (!directory)
      directory = folder->mkdir(item.first);
    directory->cd();
    entry.all->Write("all_nonzero");
    entry.multiplicity->Write("multiplicity");
    entry.edtmTime->Write("edtm_time");
    entry.edtmMultiplicity->Write("edtm_multiplicity");
    for (int index = 0; index < 3; ++index)
      entry.mult[index]->Write(Form("mult%d", index + 1));
    TNamed("time_variable", entry.timeVariable.Data()).Write();
    TNamed("multiplicity_variable", entry.multVariable.Data()).Write();
    TNamed("edtm_definition", entry.edtmDefinition.Data()).Write();
    TParameter<Long64_t>("events", entry.events).Write();
    TParameter<Long64_t>("edtm_monitor_hits", entry.edtmHits).Write();
    TParameter<Long64_t>("nonzero_multiplicity", entry.nonzero).Write();
    TParameter<Long64_t>("zero_time_with_nonzero_mult", entry.zeroTime).Write();
  }
  output->cd();
}

ReferenceEventMapLad load_reference_event_data_lad(TFile *input, const TString &spec) {
  ReferenceEventMapLad data;
  auto parent = input->GetDirectory("histograms");
  if (!parent)
    return data;
  const TString side = spec == "hms" ? "h" : "p";
  vector<TString> names = {side + "T1", side + "T2",
                           side + "FADC_TREF_ROC" + (spec == "hms" ? "1" : "2")};
  for (int index = 1; index <= (spec == "hms" ? 5 : 10); ++index)
    names.push_back(side + Form("DCREF%d", index));
  auto runs = dynamic_cast<TParameter<int> *>(input->Get("combined_run_count"));
  auto runList = dynamic_cast<TNamed *>(input->Get("combined_runs"));
  for (const auto &name : names) {
    auto directory = parent->GetDirectory(name);
    if (!directory)
      throw runtime_error("Missing saved reference: " + string(name.Data()));
    auto &entry = data[name];
    auto copy = [&](const char *key) {
      auto source = dynamic_cast<TH1F *>(directory->Get(key));
      if (!source)
        throw runtime_error("Missing saved reference histogram");
      auto result = (TH1F *)source->Clone(name + "_" + key + "_loaded");
      result->SetDirectory(nullptr);
      return result;
    };
    auto number = [&](const char *key) {
      auto value = dynamic_cast<TParameter<Long64_t> *>(directory->Get(key));
      if (!value)
        throw runtime_error("Missing saved reference metadata");
      return value->GetVal();
    };
    auto label = [&](const char *key) {
      auto value = dynamic_cast<TNamed *>(directory->Get(key));
      if (!value)
        throw runtime_error("Missing saved reference variable name");
      return TString(value->GetTitle());
    };
    entry.all = copy("all_nonzero");
    entry.multiplicity = copy("multiplicity");
    entry.edtmTime = copy("edtm_time");
    entry.edtmMultiplicity = copy("edtm_multiplicity");
    for (int m = 1; m <= 3; ++m)
      entry.mult[m - 1] = copy(Form("mult%d", m));
    entry.timeVariable = label("time_variable");
    entry.multVariable = label("multiplicity_variable");
    entry.edtmDefinition = label("edtm_definition");
    entry.events = number("events");
    entry.edtmHits = number("edtm_monitor_hits");
    entry.nonzero = number("nonzero_multiplicity");
    entry.zeroTime = number("zero_time_with_nonzero_mult");
    entry.combined = runs && runs->GetVal() > 1;
    entry.combinedRuns = runs ? runs->GetVal() : 1;
    if (runList)
      entry.combinedRunList = runList->GetTitle();
  }
  return data;
}

ReferenceEventMapLad make_reference_event_data_lad(TFile *input, TFile *output,
                                                   const TString &spec) {
  ReferenceEventMapLad data;
  auto tree = dynamic_cast<TTree *>(input->Get("T"));
  // Combined data retain the all-multiplicity and EDTM event subsets.
  if (!tree) {
    data = load_reference_event_data_lad(input, spec);
    if (!data.empty())
      write_reference_event_data_lad(output, data);
    return data;
  }
  const TString side = spec == "hms" ? "h" : "p";
  TString variablePrefix = "T." + spec + ".";
  if (!tree->GetLeaf(variablePrefix + side + "EDTM_tdcMultiplicity") &&
      tree->GetLeaf("T.coin." + side + "EDTM_tdcMultiplicity"))
    variablePrefix = "T.coin.";
  const TString edtmPrefix = variablePrefix + side + "EDTM";
  const TString edtmMultName = edtmPrefix + "_tdcMultiplicity";
  const TString edtmRawName = edtmPrefix + "_tdcTimeRaw";
  auto edtmMult = tree->GetLeaf(edtmMultName);
  auto edtmRaw = tree->GetLeaf(edtmRawName);
  if (!edtmMult || !edtmRaw)
    throw runtime_error("Missing EDTM branches; cannot label their absence as zero hits");

  vector<TString> names = {side + "T1", side + "T2",
                           side + "FADC_TREF_ROC" + (spec == "hms" ? "1" : "2")};
  for (int index = 1; index <= (spec == "hms" ? 5 : 10); ++index)
    names.push_back(side + Form("DCREF%d", index));
  tree->SetBranchStatus("*", 0);
  auto enable = [&](TLeaf *leaf) {
    if (!leaf || !leaf->GetBranch())
      throw runtime_error("Missing reference branch");
    tree->SetBranchStatus(leaf->GetBranch()->GetName(), 1);
  };
  enable(edtmMult);
  enable(edtmRaw);
  for (const auto &name : names) {
    auto &entry = data[name];
    entry.edtmDefinition = edtmMultName + ">0 && " + edtmRawName + ">0";
    const bool adc = name.Contains("FADC");
    const TString prefix = variablePrefix + name;
    entry.timeVariable = prefix + (adc ? "_adcPulseTimeRaw" : "_tdcTimeRaw");
    entry.multVariable = prefix + (adc ? "_adcMultiplicity" : "_tdcMultiplicity");
    entry.timeLeaf = tree->GetLeaf(entry.timeVariable);
    entry.multLeaf = tree->GetLeaf(entry.multVariable);
    if (!entry.timeLeaf || !entry.multLeaf)
      throw runtime_error("Missing full reference data: " + string(prefix.Data()));
    enable(entry.timeLeaf);
    enable(entry.multLeaf);
    // Broad storage prevents overflow; displayed fixed axes remain unchanged.
    entry.all = new TH1F(name + "_all_source", ";Raw reference time (channel);Counts / 2 channels",
                         262144, 0, 524288);
    entry.multiplicity = new TH1F(name + "_mult_source", ";Multiplicity;Counts / 1", 9, -.5, 8.5);
    entry.all->SetDirectory(nullptr);
    entry.multiplicity->SetDirectory(nullptr);
    for (int index = 0; index < 3; ++index) {
      entry.mult[index] = (TH1F *)entry.all->Clone(name + Form("_mult%d_source", index + 1));
      entry.mult[index]->SetDirectory(nullptr);
    }
    entry.edtmTime = (TH1F *)entry.all->Clone(name + "_edtm_time_source");
    entry.edtmMultiplicity = (TH1F *)entry.multiplicity->Clone(name + "_edtm_mult_source");
    entry.edtmTime->SetDirectory(nullptr);
    entry.edtmMultiplicity->SetDirectory(nullptr);
    style_edtm_reference_lad(entry.edtmTime);
    style_edtm_reference_lad(entry.edtmMultiplicity);
  }
  for (Long64_t index = 0; index < tree->GetEntries(); ++index) {
    if (tree->GetEntry(index) < 0)
      throw runtime_error("Cannot read reference event");
    const bool tagged = edtmMult->GetValue() > 0 && edtmRaw->GetValue() > 0;
    for (auto &item : data) {
      auto &entry = item.second;
      if (entry.timeLeaf->GetNdata() != 1 || entry.multLeaf->GetNdata() != 1)
        throw runtime_error("Expected scalar selected reference time and multiplicity");
      const int mult = lround(entry.multLeaf->GetValue());
      const double time = entry.timeLeaf->GetValue();
      ++entry.events;
      entry.multiplicity->Fill(mult);
      if (tagged) {
        ++entry.edtmHits;
        entry.edtmMultiplicity->Fill(mult);
      }
      if (mult <= 0)
        continue;
      ++entry.nonzero;
      entry.all->Fill(time);
      if (time == 0)
        ++entry.zeroTime;
      if (mult <= 3)
        entry.mult[mult - 1]->Fill(time);
      if (tagged)
        entry.edtmTime->Fill(time);
    }
  }
  tree->SetBranchStatus("*", 1);
  write_reference_event_data_lad(output, data);
  return data;
}

const ReferenceEventDataLad *reference_data_lad(const ReferenceEventMapLad &data,
                                                const TString &name) {
  auto found = data.find(name);
  return found == data.end() ? nullptr : &found->second;
}

struct MultiplicityOverlayLad {
  TH1F *allNonzero;
  TH1F *selectedSum;
  TString selectedLabel;
};

void style_reference_cut_lad(TLine *cutLine) {
  if (!cutLine)
    return;
  cutLine->SetLineColor(kOrange);
  cutLine->SetLineWidth(1);
  cutLine->SetLineStyle(1);
}

TH1F *multiplicity_histogram_lad(int multiplicity, TH1F *mult1, TH1F *mult2, TH1F *mult3) {
  if (multiplicity == 1)
    return mult1;
  if (multiplicity == 2)
    return mult2;
  if (multiplicity == 3)
    return mult3;
  return nullptr;
}

double multiplicity_count_lad(TH1F *multiplicity, int value) {
  if (!multiplicity)
    return 0.0;
  return multiplicity->GetBinContent(multiplicity->GetXaxis()->FindBin(value));
}

MultiplicityOverlayLad build_multiplicity_overlays_lad(const TString &name, TH1F *multiplicity,
                                                       TH1F *mult1, TH1F *mult2, TH1F *mult3) {
  MultiplicityOverlayLad result;
  result.allNonzero = nullptr;
  result.selectedSum = nullptr;
  TH1F *source = mult1 ? mult1 : (mult2 ? mult2 : mult3);
  if (!source)
    return result;

  result.allNonzero = (TH1F *)source->Clone(name + "_all_nonzero");
  result.allNonzero->SetDirectory(nullptr);
  result.allNonzero->Reset("ICES");
  if (mult1)
    result.allNonzero->Add(mult1);
  if (mult2)
    result.allNonzero->Add(mult2);
  if (mult3)
    result.allNonzero->Add(mult3);
  result.allNonzero->SetLineColor(kBlue);
  result.allNonzero->SetLineWidth(2);

  vector<pair<double, int>> ranked;
  for (int mult = 1; mult <= 3; ++mult) {
    const double count = multiplicity_count_lad(multiplicity, mult);
    if (count > 0.0)
      ranked.push_back(make_pair(count, mult));
  }
  sort(ranked.begin(), ranked.end(),
       [](const pair<double, int> &left, const pair<double, int> &right) {
         if (left.first != right.first)
           return left.first > right.first;
         return left.second < right.second;
       });
  if (ranked.empty())
    return result;

  const double largestCount = ranked.front().first;
  vector<int> selectedMultiplicities;
  for (const pair<double, int> &entry : ranked) {
    if (ladplot::number("REF_MULT_RATIO", 10.0) * entry.first > largestCount)
      selectedMultiplicities.push_back(entry.second);
  }
  sort(selectedMultiplicities.begin(), selectedMultiplicities.end());

  result.selectedSum = (TH1F *)source->Clone(name + "_selected_sum");
  result.selectedSum->SetDirectory(nullptr);
  result.selectedSum->Reset("ICES");
  for (int mult : selectedMultiplicities) {
    TH1F *component = multiplicity_histogram_lad(mult, mult1, mult2, mult3);
    if (component)
      result.selectedSum->Add(component);
    if (result.selectedLabel.IsNull())
      result.selectedLabel = Form("mult = %d", mult);
    else
      result.selectedLabel += Form(" + %d", mult);
  }
  result.selectedSum->SetLineColor(kRed);
  result.selectedSum->SetLineWidth(2);
  return result;
}

TH1F *make_multiplicity_display_lad(const TString &name, const TString &title, TH1F *source) {
  TH1F *display = new TH1F(name, title + ";Multiplicity;Counts / 1", 9, -0.5, 8.5);
  display->SetDirectory(nullptr);
  for (int mult = 0; mult <= 8; ++mult) {
    const int targetBin = display->FindBin(mult);
    const int sourceBin = source->GetXaxis()->FindBin(mult);
    display->SetBinContent(targetBin, source->GetBinContent(sourceBin));
    display->SetBinError(targetBin, source->GetBinError(sourceBin));
  }
  display->SetBinContent(0, source->GetBinContent(0));
  display->SetBinContent(10, source->GetBinContent(source->GetNbinsX() + 1));
  display->SetEntries(source->GetEntries());
  display->SetLineColor(kBlue + 1);
  display->SetFillStyle(0);
  display->SetLineWidth(2);
  display->SetStats(1);
  return display;
}

pair<double, double> automatic_range_lad(TH1F *histogram, int minEntries, double fixedMax) {
  if (!histogram || histogram->GetEntries() <= 0.0)
    return make_pair(0.0, fixedMax);
  int firstBin = histogram->FindFirstBinAbove(minEntries, 1, 1, histogram->GetNbinsX());
  int lastBin = histogram->FindLastBinAbove(minEntries, 1, 1, histogram->GetNbinsX());
  if (firstBin < 1 || lastBin < firstBin) {
    firstBin = histogram->FindFirstBinAbove(0.0, 1, 1, histogram->GetNbinsX());
    lastBin = histogram->FindLastBinAbove(0.0, 1, 1, histogram->GetNbinsX());
  }
  if (firstBin < 1 || lastBin < firstBin)
    return make_pair(0.0, fixedMax);
  const double first = histogram->GetXaxis()->GetBinLowEdge(firstBin);
  const double last = histogram->GetXaxis()->GetBinUpEdge(lastBin);
  double low = 0.9 * first;
  double high = 1.1 * last;
  if (high <= low) {
    low = first - 50.0;
    high = last + 50.0;
  }
  return make_pair(low, high);
}

struct ReferencePeakLad {
  int first = 0, last = 0, occupied = 0;
  double count = 0;
};

vector<pair<double, double>> reference_peak_support_lad(TH1F *histogram) {
  vector<ReferencePeakLad> groups;
  if (!histogram || histogram->GetEntries() == 0)
    return {};
  double maximum = 0;
  for (int b = 1; b <= histogram->GetNbinsX(); ++b)
    maximum = max(maximum, histogram->GetBinContent(b));
  double threshold = max(3., .002 * maximum);
  ReferencePeakLad group;
  auto close = [&]() {
    if (group.first == 0)
      return;
    int low = max(1, group.first - 2), high = min(histogram->GetNbinsX(), group.last + 2);
    group.count = histogram->Integral(low, high);
    groups.push_back(group);
    group = ReferencePeakLad();
  };
  for (int b = 1; b <= histogram->GetNbinsX(); ++b)
    if (histogram->GetBinContent(b) >= threshold) {
      if (group.first && b - group.last > 6)
        close();
      if (!group.first)
        group.first = b;
      group.last = b;
      ++group.occupied;
    }
  close();
  if (groups.empty())
    return {};
  sort(groups.begin(), groups.end(),
       [](const auto &a, const auto &b) { return a.count > b.count; });
  vector<pair<double, double>> result;
  for (size_t index = 0; index < groups.size(); ++index) {
    const auto &peak = groups[index];
    const bool significant = index == 0 || peak.count >= .02 * groups.front().count ||
                             (peak.count >= max(50., .002 * groups.front().count) &&
                              peak.occupied >= 3 && peak.last - peak.first >= 2);
    if (!significant)
      continue;
    result.push_back(
        {histogram->GetXaxis()->GetBinLowEdge(max(1, peak.first - 2)),
         histogram->GetXaxis()->GetBinUpEdge(min(histogram->GetNbinsX(), peak.last + 2))});
  }
  return result;
}

pair<double, double> detailed_reference_range_lad(TH1F *all, TH1F *edtm, const vector<double> &cuts,
                                                  int minEntries, double fixedMax) {
  auto peaks = reference_peak_support_lad(all);
  auto taggedPeaks = reference_peak_support_lad(edtm);
  peaks.insert(peaks.end(), taggedPeaks.begin(), taggedPeaks.end());
  if (peaks.empty())
    return automatic_range_lad(all, minEntries, fixedMax);
  double low = peaks.front().first, high = peaks.front().second;
  for (auto peak : peaks) {
    low = min(low, peak.first);
    high = max(high, peak.second);
  }
  const double signalWidth = max(2., high - low);
  for (double cut : cuts)
    if (cut >= low - max(150., 2 * signalWidth) && cut <= high + max(150., 2 * signalWidth)) {
      low = min(low, cut);
      high = max(high, cut);
    }
  // Approximately 1200 channels, centered on the resolved peak envelope.
  // Widen only if necessary to retain separated peaks and nearby cut markers.
  const double padding = max(12., .12 * (high - low));
  const double width =
      2 * ceil(max(ladplot::number("REF_DETAIL_WIDTH", 1200.), high - low + 2 * padding) / 2);
  const double center = .5 * (low + high);
  const double left = max(0., 2 * floor((center - .5 * width) / 2));
  return {left, left + width};
}

// The plot frame contains the legend, while every curve and cut stays below it.
double reference_display_maximum_lad(double maximum, bool logarithmic, bool cutLines) {
  double obstacle = maximum > 0 ? maximum : 1.;
  if (cutLines)
    obstacle = maximum > 0 ? 1.15 * maximum : 1.;
  const double dataFraction = (.72 - .18) / (1. - .10 - .18);
  return logarithmic ? .5 * pow(obstacle / .5, 1. / dataFraction) : obstacle / dataFraction;
}

double maximum_in_range_lad(TH1F *histogram, double low, double high) {
  if (!histogram)
    return 0.0;
  const int first = max(1, histogram->GetXaxis()->FindBin(low));
  const int last = min(histogram->GetNbinsX(), histogram->GetXaxis()->FindBin(high));
  double maximum = 0.0;
  for (int bin = first; bin <= last; ++bin)
    maximum = max(maximum, histogram->GetBinContent(bin));
  return maximum;
}

void draw_reference_panel_lad(const TString &name, const TString &title, TH1F *multiplicity,
                              TH1F *mult1, TH1F *mult2, TH1F *mult3, double fixedMax,
                              int minEntries, const vector<double> &cuts, bool automatic,
                              const ReferenceEventDataLad *eventData = nullptr) {
  MultiplicityOverlayLad overlays =
      build_multiplicity_overlays_lad(name, multiplicity, mult1, mult2, mult3);
  if (eventData) {
    delete overlays.allNonzero;
    overlays.allNonzero = (TH1F *)eventData->all->Clone(name + "_all_nonzero");
    overlays.allNonzero->SetDirectory(nullptr);
    overlays.allNonzero->SetLineColor(kBlue);
    overlays.allNonzero->SetLineWidth(2);
  }
  if (!overlays.allNonzero) {
    cout << "Missing nonzero-multiplicity histograms for " << title << endl;
    return;
  }
  pair<double, double> range = automatic
                                   ? automatic_range_lad(overlays.allNonzero, minEntries, fixedMax)
                                   : make_pair(0.0, fixedMax);
  // Individual runs retain the original automatic range; combined plots resolve peaks.
  if (automatic && eventData && eventData->combined)
    range = detailed_reference_range_lad(overlays.allNonzero, eventData->edtmTime, cuts, minEntries,
                                         fixedMax);
  // Legends sit inside the frame, above all curves and cut-line endpoints.
  gPad->SetLeftMargin(.16);
  gPad->SetRightMargin(.06);
  gPad->SetBottomMargin(.18);
  gPad->SetTopMargin(.10);
  TH1F *all = overlays.allNonzero;
  all->SetTitle(title + (automatic ? (eventData && eventData->combined ? " - Detailed range"
                                                                       : " - Auto range")
                                   : " - Fixed range"));
  all->SetStats(0);
  all->GetXaxis()->SetRangeUser(range.first, range.second);
  all->GetXaxis()->SetNdivisions(505);
  all->GetXaxis()->SetNoExponent(true);
  all->SetMinimum(0.5);
  const double maximum = maximum_in_range_lad(all, range.first, range.second);
  all->SetMaximum(reference_display_maximum_lad(maximum, true, !cuts.empty()));
  all->Draw("HIST");
  if (overlays.selectedSum) {
    overlays.selectedSum->GetXaxis()->SetRangeUser(range.first, range.second);
    overlays.selectedSum->Draw("HIST SAME");
  }
  const double lineTop = maximum > 0.0 ? 1.15 * maximum : 1.0;
  for (double cut : cuts) {
    TLine *cutLine = new TLine(cut, 0.5, cut, lineTop);
    style_reference_cut_lad(cutLine);
    cutLine->Draw("SAME");
  }
  TLegend *legend = new TLegend(0.18, 0.75, 0.58, 0.88);
  legend->SetBorderSize(0);
  legend->SetFillStyle(0);
  legend->SetTextSize(0.029);
  legend->AddEntry(all, "mult > 0", "l");
  if (overlays.selectedSum)
    legend->AddEntry(overlays.selectedSum, overlays.selectedLabel, "l");
  TH1F *tagged = nullptr;
  if (eventData && eventData->edtmTime->GetEntries() > 0) {
    tagged = (TH1F *)eventData->edtmTime->Clone(name + "_edtm_overlay");
    tagged->SetDirectory(nullptr);
    style_edtm_reference_lad(tagged);
    tagged->GetXaxis()->SetRangeUser(range.first, range.second);
    legend->AddEntry(tagged, "EDTM events", "l");
  }
  legend->Draw();
  // Last primitive: green dashed EDTM subset remains above every other curve.
  if (tagged)
    tagged->Draw("HIST SAME");
}

TCanvas *make_reference_page_lad(const TString &name, const TString &title, TH1F *multiplicity,
                                 TH1F *mult1, TH1F *mult2, TH1F *mult3, double fixedMax,
                                 int minEntries, const vector<double> &cuts,
                                 const ReferenceEventDataLad *eventData = nullptr) {
  if (name.Contains("ROC"))
    fixedMax = ladplot::number("REF_ROC_MAX", 10000.);
  else if (name.BeginsWith("hT"))
    fixedMax = ladplot::number("REF_HMS_T_MAX", 50000.);
  else if (name.BeginsWith("pT"))
    fixedMax = ladplot::number("REF_SHMS_T_MAX", 30000.);
  else
    fixedMax = ladplot::number("REF_DC_MAX", 30000.);
  if (eventData) {
    multiplicity = eventData->multiplicity;
    mult1 = eventData->mult[0];
    mult2 = eventData->mult[1];
    mult3 = eventData->mult[2];
  }
  if (!multiplicity || !mult1 || !mult2 || !mult3) {
    cout << "Missing histogram(s) for " << title << endl;
    return nullptr;
  }
  TString displayTitle = title;
  if (eventData && eventData->combined) {
    const auto position = displayTitle.Index(", run ");
    if (position != kNPOS)
      displayTitle.Remove(position);
    displayTitle += Form(", combined %d runs", eventData->combinedRuns);
  }
  TCanvas *canvas = new TCanvas(name, displayTitle, 1980, 700);
  canvas->Divide(3, 1, 0.025, 0.01);
  canvas->cd(1);
  TH1F *multDisplay = make_multiplicity_display_lad(name + "_multiplicity",
                                                    displayTitle + " Multiplicity", multiplicity);
  gPad->SetLeftMargin(.16);
  gPad->SetRightMargin(.06);
  gPad->SetBottomMargin(.18);
  gPad->SetTopMargin(.10);
  gStyle->SetStatX(.94);
  gStyle->SetStatY(.88);
  gStyle->SetStatW(.23);
  gStyle->SetStatH(.14);
  multDisplay->SetMaximum(reference_display_maximum_lad(multDisplay->GetMaximum(), false, false));
  multDisplay->Draw("HIST");
  auto legend = new TLegend(.18, .78, .58, .88);
  legend->SetBorderSize(0);
  legend->SetFillStyle(0);
  legend->SetTextSize(.029);
  legend->AddEntry(multDisplay, "All events", "l");
  if (eventData && eventData->edtmHits > 0) {
    auto tagged = (TH1F *)eventData->edtmMultiplicity->Clone(name + "_multiplicity_edtm_overlay");
    tagged->SetDirectory(nullptr);
    style_edtm_reference_lad(tagged);
    legend->AddEntry(tagged, "EDTM events", "l");
    legend->Draw();
    tagged->Draw("HIST SAME");
  } else
    legend->Draw();
  canvas->cd(2);
  gPad->SetLogy();
  draw_reference_panel_lad(name + "_fixed", displayTitle, multiplicity, mult1, mult2, mult3,
                           fixedMax, minEntries, cuts, false, eventData);
  canvas->cd(3);
  gPad->SetLogy();
  draw_reference_panel_lad(name + "_auto", displayTitle, multiplicity, mult1, mult2, mult3,
                           fixedMax, minEntries, cuts, true, eventData);
  canvas->Update();
  return canvas;
}

void write_reference_outputs_lad(TFile *output, vector<TCanvas *> &canvases,
                                 const TString &outfile) {
  gStyle->SetPaperSize(29.7, 10.5);
  output->cd();
  for (TCanvas *canvas : canvases)
    if (canvas)
      canvas->Write();
  output->Write();
  output->Close();
  TString pdfFile = outfile;
  pdfFile.ReplaceAll(".root", ".pdf");
  vector<TCanvas *> valid;
  for (TCanvas *canvas : canvases)
    if (canvas)
      valid.push_back(canvas);
  ladplot::exportPDF(outfile, pdfFile, ladplot::canvases(outfile));
}

void run_shms_reference_time_setup(TString infile, int RunNumber, TString outfile = "move_me.root",
                                   TString spec = "shms") {
  gROOT->SetBatch(kTRUE);
  gHcParms->Define("gen_run_number", "Run Number", RunNumber);
  if (spec.CompareTo("shms", TString::kIgnoreCase) == 0) {
    gHcParms->AddString(
        "g_ctp_database_filename",
        ladplot::setting("PLOT_DATABASE", "DBASE/LAD_COIN/standard.database").Data());
    gHcParms->Load(gHcParms->GetString("g_ctp_database_filename"), RunNumber);
    gHcParms->Load(gHcParms->GetString("g_ctp_parm_filename"));
    ladplot::overrideParameters();
    gHcParms->Load(gHcParms->GetString("g_ctp_kinematics_filename"), RunNumber);
    gHcParms->Load("PARAM/TRIG/tshms.param");
  } else if (spec.CompareTo("coin", TString::kIgnoreCase) == 0) {
    gHcParms->AddString(
        "g_ctp_database_filename",
        ladplot::setting("PLOT_DATABASE", "DBASE/LAD_COIN/standard.database").Data());
    gHcParms->Load(gHcParms->GetString("g_ctp_database_filename"), RunNumber);
    gHcParms->Load(gHcParms->GetString("g_ctp_parm_filename"));
    ladplot::overrideParameters();
    gHcParms->Load(gHcParms->GetString("g_ctp_kinematics_filename"), RunNumber);
    gHcParms->Load("PARAM/TRIG/tcoin.param");
    gHcParms->Load("PARAM/HMS/GEN/h_fadc_debug.param");
    gHcParms->Load("PARAM/SHMS/GEN/p_fadc_debug.param");
  } else {
    cout << "Unknown Spec: " << spec << endl;
    return;
  }

  Int_t fshms_tdcrefcut[1], fdc_tdcrefcut[1], fhodo_tdcrefcut[1];
  Int_t fhodo_adcrefcut[1], fngcer_adcrefcut[1], fhgcer_adcrefcut[1];
  Int_t faero_adcrefcut[1], fcal_adcrefcut[1];
  DBRequest windowList[] = {{"dc_tdcrefcut", fdc_tdcrefcut, kInt, 1, 1},
                            {"hodo_tdcrefcut", fhodo_tdcrefcut, kInt, 1, 1},
                            {"hodo_adcrefcut", fhodo_adcrefcut, kInt, 1, 1},
                            {"ngcer_adcrefcut", fngcer_adcrefcut, kInt, 1, 1},
                            {"hgcer_adcrefcut", fhgcer_adcrefcut, kInt, 1, 1},
                            {"aero_adcrefcut", faero_adcrefcut, kInt, 1, 1},
                            {"cal_adcrefcut", fcal_adcrefcut, kInt, 1, 1},
                            {0}};
  DBRequest trigRefList[] = {{Form("_%s_trig_tdcrefcut", spec.Data()), fshms_tdcrefcut, kInt, 1, 1},
                             {0}};
  gHcParms->LoadParmValues((DBRequest *)&trigRefList, "t");
  gHcParms->LoadParmValues((DBRequest *)&windowList, "p");

  gStyle->SetOptStat(1111);
  TFile *input = new TFile(infile, "READ");
  if (input->IsZombie()) {
    cout << "Cannot find: " << infile << endl;
    return;
  }
  TFile *output = new TFile(outfile, "RECREATE");
  const auto eventData = make_reference_event_data_lad(input, output, "shms");
  vector<TCanvas *> pages;
  for (int index = 1; index <= 2; ++index) {
    vector<double> cuts(1, index == 1 ? -fhodo_tdcrefcut[0] : -fshms_tdcrefcut[0]);
    pages.push_back(make_reference_page_lad(
        Form("pT%d_page", index), Form("SHMS pT%d Reference Time, run %d", index, RunNumber),
        (TH1F *)input->Get(Form("pT%d_tdc_mult", index)),
        (TH1F *)input->Get(Form("pT%d_tdc_time_raw_mult1", index)),
        (TH1F *)input->Get(Form("pT%d_tdc_time_raw_mult2", index)),
        (TH1F *)input->Get(Form("pT%d_tdc_time_raw_mult3", index)), 30000.0, 5, cuts,
        reference_data_lad(eventData, Form("pT%d", index))));
  }
  vector<double> rocCuts;
  rocCuts.push_back(-faero_adcrefcut[0]);
  rocCuts.push_back(-fhodo_adcrefcut[0]);
  rocCuts.push_back(-fcal_adcrefcut[0]);
  rocCuts.push_back(-fngcer_adcrefcut[0]);
  rocCuts.push_back(-fhgcer_adcrefcut[0]);
  pages.push_back(make_reference_page_lad(
      "pROC2_page", Form("SHMS ROC2 Reference Time, run %d", RunNumber),
      (TH1F *)input->Get("pFADC_TREF_ROC2_adc_mult"), (TH1F *)input->Get("pFADC_TREF_ROC2_mult1"),
      (TH1F *)input->Get("pFADC_TREF_ROC2_mult2"), (TH1F *)input->Get("pFADC_TREF_ROC2_mult3"),
      10000.0, 5, rocCuts, reference_data_lad(eventData, "pFADC_TREF_ROC2")));
  for (int index = 1; index <= 10; ++index) {
    vector<double> cuts(1, -fdc_tdcrefcut[0]);
    pages.push_back(make_reference_page_lad(
        Form("pDC%d_page", index), Form("SHMS pDC%d Reference Time, run %d", index, RunNumber),
        (TH1F *)input->Get(Form("pdc_tdc_mult_ref%d", index)),
        (TH1F *)input->Get(Form("pdc_time_raw_ref%d_mult1", index)),
        (TH1F *)input->Get(Form("pdc_time_raw_ref%d_mult2", index)),
        (TH1F *)input->Get(Form("pdc_time_raw_ref%d_mult3", index)), 30000.0, 5, cuts,
        reference_data_lad(eventData, Form("pDCREF%d", index))));
  }
  write_reference_outputs_lad(output, pages, outfile);
  input->Close();
}

void run_hms_reference_time_setup(TString infile, int RunNumber, TString outfile = "move_me.root",
                                  TString spec = "hms") {
  gROOT->SetBatch(kTRUE);
  gHcParms->Define("gen_run_number", "Run Number", RunNumber);
  if (spec.CompareTo("hms", TString::kIgnoreCase) == 0) {
    gHcParms->AddString(
        "g_ctp_database_filename",
        ladplot::setting("PLOT_DATABASE", "DBASE/LAD_COIN/standard.database").Data());
    gHcParms->Load(gHcParms->GetString("g_ctp_database_filename"), RunNumber);
    gHcParms->Load(gHcParms->GetString("g_ctp_parm_filename"));
    ladplot::overrideParameters();
    gHcParms->Load(gHcParms->GetString("g_ctp_kinematics_filename"), RunNumber);
    gHcParms->Load("PARAM/TRIG/thms.param");
    gHcParms->Load("PARAM/HMS/GEN/h_fadc_debug.param");
  } else if (spec.CompareTo("coin", TString::kIgnoreCase) == 0) {
    gHcParms->AddString("g_ctp_database_filename", "DBASE/COIN/standard.database");
    gHcParms->Load(gHcParms->GetString("g_ctp_database_filename"), RunNumber);
    gHcParms->Load(gHcParms->GetString("g_ctp_parm_filename"));
    ladplot::overrideParameters();
    gHcParms->Load(gHcParms->GetString("g_ctp_kinematics_filename"), RunNumber);
    gHcParms->Load("PARAM/TRIG/tcoin.param");
    gHcParms->Load("PARAM/HMS/GEN/h_fadc_debug.param");
    gHcParms->Load("PARAM/SHMS/GEN/p_fadc_debug.param");
  } else {
    cout << "Unknown Spec: " << spec << endl;
    return;
  }

  Int_t fhms_tdcrefcut[1], fdc_tdcrefcut[1], fhodo_tdcrefcut[1];
  Int_t fhodo_adcrefcut[1], fcer_adcrefcut[1], fcal_adcrefcut[1];
  DBRequest windowList[] = {{"dc_tdcrefcut", fdc_tdcrefcut, kInt, 1, 1},
                            {"hodo_tdcrefcut", fhodo_tdcrefcut, kInt, 1, 1},
                            {"hodo_adcrefcut", fhodo_adcrefcut, kInt, 1, 1},
                            {"cer_adcrefcut", fcer_adcrefcut, kInt, 1, 1},
                            {"cal_adcrefcut", fcal_adcrefcut, kInt, 1, 1},
                            {0}};
  DBRequest trigRefList[] = {{Form("_%s_trig_tdcrefcut", spec.Data()), fhms_tdcrefcut, kInt, 1, 1},
                             {0}};
  gHcParms->LoadParmValues((DBRequest *)&trigRefList, "t");
  gHcParms->LoadParmValues((DBRequest *)&windowList, "h");

  gStyle->SetOptStat(1111);
  TFile *input = new TFile(infile, "READ");
  if (input->IsZombie()) {
    cout << "Cannot find: " << infile << endl;
    return;
  }
  TFile *output =
      new TFile(outfile, spec.CompareTo("coin", TString::kIgnoreCase) == 0 ? "UPDATE" : "RECREATE");
  const auto eventData = make_reference_event_data_lad(input, output, "hms");
  vector<TCanvas *> pages;
  for (int index = 1; index <= 2; ++index) {
    vector<double> cuts(1, index == 1 ? -fhms_tdcrefcut[0] : -fhodo_tdcrefcut[0]);
    pages.push_back(make_reference_page_lad(
        Form("hT%d_page", index), Form("HMS hT%d Reference Time, run %d", index, RunNumber),
        (TH1F *)input->Get(Form("hT%d_tdc_mult", index)),
        (TH1F *)input->Get(Form("hT%d_tdc_time_raw_mult1", index)),
        (TH1F *)input->Get(Form("hT%d_tdc_time_raw_mult2", index)),
        (TH1F *)input->Get(Form("hT%d_tdc_time_raw_mult3", index)), 50000.0, 10, cuts,
        reference_data_lad(eventData, Form("hT%d", index))));
  }
  vector<double> rocCuts;
  rocCuts.push_back(-fhodo_adcrefcut[0]);
  rocCuts.push_back(-fcal_adcrefcut[0]);
  rocCuts.push_back(-fcer_adcrefcut[0]);
  pages.push_back(make_reference_page_lad(
      "hROC1_page", Form("HMS ROC1 Reference Time, run %d", RunNumber),
      (TH1F *)input->Get("hFADC_TREF_ROC1_adc_mult"), (TH1F *)input->Get("hFADC_TREF_ROC1_mult1"),
      (TH1F *)input->Get("hFADC_TREF_ROC1_mult2"), (TH1F *)input->Get("hFADC_TREF_ROC1_mult3"),
      10000.0, 10, rocCuts, reference_data_lad(eventData, "hFADC_TREF_ROC1")));
  for (int index = 1; index <= 5; ++index) {
    if (!input->Get(Form("hdc_time_raw_ref%d_mult1", index)) &&
        !reference_data_lad(eventData, Form("hDCREF%d", index))) {
      cout << "No hDC reference " << index << " in this replay" << endl;
      continue;
    }
    vector<double> cuts(1, -fdc_tdcrefcut[0]);
    pages.push_back(make_reference_page_lad(
        Form("hDC%d_page", index), Form("HMS hDC%d Reference Time, run %d", index, RunNumber),
        (TH1F *)input->Get(Form("hdc_tdc_mult_ref%d", index)),
        (TH1F *)input->Get(Form("hdc_time_raw_ref%d_mult1", index)),
        (TH1F *)input->Get(Form("hdc_time_raw_ref%d_mult2", index)),
        (TH1F *)input->Get(Form("hdc_time_raw_ref%d_mult3", index)), 30000.0, 9, cuts,
        reference_data_lad(eventData, Form("hDCREF%d", index))));
  }
  write_reference_outputs_lad(output, pages, outfile);
  input->Close();
}

void run_coin_reference_time_setup(TString infile, int RunNumber,
                                   TString outfile = "move_me.root") {
  run_shms_reference_time_setup(infile, RunNumber, outfile, "coin");
  run_hms_reference_time_setup(infile, RunNumber, outfile, "coin");
}
