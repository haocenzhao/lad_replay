#pragma once
#include <TFile.h>
#include <TCanvas.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TSystem.h>
#include <THcGlobals.h>
#include <THcParmList.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
namespace ladplot {
inline TString setting(const char *name, const char *fallback = "") {
  auto v = gSystem->Getenv(name);
  return v && *v ? TString(v) : TString(fallback);
}
inline double number(const char *name, double fallback) {
  auto v = setting(name);
  return v.IsNull() ? fallback : std::stod(v.Data());
}
inline bool flag(const char *name, bool fallback) { return number(name, fallback ? 1 : 0) != 0; }
inline TString quote(TString text) {
  text.ReplaceAll("'", "'\\''");
  return "'" + text + "'";
}
inline void parameters(int run) {
  gHcParms->Define("gen_run_number", "Run Number", run);
  TString db = setting("PLOT_DATABASE", "DBASE/LAD_COIN/standard.database");
  gHcParms->Load(db, run);
  gHcParms->Load(gHcParms->GetString("g_ctp_parm_filename"));
  auto extra = setting("PARAM_OVERRIDE");
  if (!extra.IsNull())
    gHcParms->Load(extra);
}
inline void overrideParameters() {
  auto extra = setting("PARAM_OVERRIDE");
  if (!extra.IsNull())
    gHcParms->Load(extra);
}
// Reopen saved canvases so PDF and ROOT use identical axes and primitives.
inline void exportPDF(TString root, TString pdf, const std::vector<TString> &names) {
  TFile file(root, "READ");
  if (file.IsZombie() || names.empty())
    throw std::runtime_error("No saved canvases to export");
  TString command = quote(setting("PYTHON_BIN", "python3")) + " " + quote(setting("PLOT_DRIVER")) +
                    " merge " + quote(pdf);
  std::vector<TString> pages;
  for (size_t i = 0; i < names.size(); ++i) {
    auto c = dynamic_cast<TCanvas *>(file.Get(names[i]));
    if (!c)
      throw std::runtime_error("Missing saved canvas: " + std::string(names[i].Data()));
    TString page =
        Form("%s/page_%d_%zu.pdf", setting("PLOT_SCRATCH", gSystem->TempDirectory()).Data(),
             gSystem->GetPid(), i);
    pid_t pid = fork();
    if (pid == 0) {
      c->Draw();
      c->Modified();
      c->Update();
      gStyle->SetPaperSize(29.7, 29.7 * double(c->GetWh()) / c->GetWw());
      c->Print(page);
      _exit(gSystem->AccessPathName(page) ? 2 : 0);
    }
    int status = 0;
    if (pid < 0 || waitpid(pid, &status, 0) < 0 || !WIFEXITED(status) || WEXITSTATUS(status))
      throw std::runtime_error("PDF page export failed");
    pages.push_back(page);
    command += " " + quote(page);
    delete c;
  }
  if (gSystem->Exec(command))
    throw std::runtime_error("PDF merge failed");
  for (auto p : pages)
    gSystem->Unlink(p);
}
inline std::vector<TString> canvases(TString root, TString directory = "") {
  TFile f(root);
  auto d = directory.IsNull() ? static_cast<TDirectory *>(&f) : f.GetDirectory(directory);
  if (!d)
    throw std::runtime_error("Missing canvas directory");
  std::vector<TString> result;
  TIter next(d->GetListOfKeys());
  while (auto key = dynamic_cast<TKey *>(next()))
    if (TString(key->GetClassName()) == "TCanvas")
      result.push_back((directory.IsNull() ? TString("") : directory + "/") + key->GetName());
  return result;
}
} // namespace ladplot
