#include <TSystem.h>
#include <set>

// Load reference_time_setup_lad.C before this macro.
void add_reference_event_data_lad(ReferenceEventMapLad &total, ReferenceEventMapLad &part) {
  if (total.empty()) {
    total = std::move(part);
    return;
  }
  if (total.size() != part.size())
    throw runtime_error("Incompatible reference maps");
  for (auto &item : total) {
    auto found = part.find(item.first);
    if (found == part.end())
      throw runtime_error("Missing combined reference");
    auto &left = item.second;
    auto &right = found->second;
    if (left.timeVariable != right.timeVariable || left.multVariable != right.multVariable ||
        left.edtmDefinition != right.edtmDefinition)
      throw runtime_error("Inconsistent combined variable definitions");
    vector<TH1F *> destination = {
        left.all,     left.multiplicity, left.edtmTime, left.edtmMultiplicity,
        left.mult[0], left.mult[1],      left.mult[2]};
    vector<TH1F *> source = {
        right.all,     right.multiplicity, right.edtmTime, right.edtmMultiplicity,
        right.mult[0], right.mult[1],      right.mult[2]};
    for (size_t index = 0; index < source.size(); ++index) {
      if (destination[index]->GetNbinsX() != source[index]->GetNbinsX() ||
          destination[index]->GetXaxis()->GetXmin() != source[index]->GetXaxis()->GetXmin() ||
          destination[index]->GetXaxis()->GetXmax() != source[index]->GetXaxis()->GetXmax())
        throw runtime_error("Incompatible combined binning");
      if (!destination[index]->Add(source[index]))
        throw runtime_error("Cannot add reference histogram");
      delete source[index];
    }
    left.events += right.events;
    left.edtmHits += right.edtmHits;
    left.nonzero += right.nonzero;
    left.zeroTime += right.zeroTime;
  }
}

void combine_reference_inputs_lad(const vector<TString> &inputs, TString spec, TString destination,
                                  const vector<int> &runs) {
  ReferenceEventMapLad total;
  for (const auto &path : inputs) {
    TFile input(path, "READ");
    if (input.IsZombie())
      throw runtime_error("Cannot read reference input");
    auto part = load_reference_event_data_lad(&input, spec);
    if (part.empty())
      throw runtime_error("Combined ref-time input must be a saved ref-time plot ROOT with "
                          "histograms/; run single-run plotting first");
    add_reference_event_data_lad(total, part);
  }
  TString list;
  for (int run : runs) {
    if (!list.IsNull())
      list += ",";
    list += Form("%d", run);
  }
  for (auto &x : total) {
    x.second.combined = true;
    x.second.combinedRuns = runs.size();
    x.second.combinedRunList = list;
  }
  TFile out(destination, "RECREATE");
  write_reference_event_data_lad(&out, total);
}
