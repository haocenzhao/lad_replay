void replay_reference_events_hms(Int_t RunNumber=0, Int_t MaxEvent=0) {

  // Get RunNumber and MaxEvent if not provided.
  if(RunNumber == 0) {
    cout << "Enter a Run Number (-1 to exit): ";
    cin >> RunNumber;
    if( RunNumber<=0 ) return;
  }
  if(MaxEvent == 0) {
    cout << "\nNumber of Events to analyze: ";
    cin >> MaxEvent;
    if(MaxEvent == 0) {
      cerr << "...Invalid entry\n";
      exit;
    }
  }

  // Create file name patterns.
  const char* RunFileNamePattern = "lad_Production_%05d.dat.0";
  vector<TString> pathList;
  if(gSystem->Getenv("REPLAY_RAW_DIR")) pathList.push_back(gSystem->Getenv("REPLAY_RAW_DIR"));
  pathList.push_back(".");
  pathList.push_back("./raw");
  pathList.push_back("./raw/../raw.copiedtotape");
  pathList.push_back("./cache");
  pathList.push_back("/cache/hallc/c-lad/raw/");

  const char* ROOTFileNamePattern = gSystem->Getenv("REPLAY_OUTPUT_PATTERN");

  // Load Global parameters
  // Add variables to global list.
  gHcParms->Define("gen_run_number", "Run Number", RunNumber);
  gHcParms->AddString("g_ctp_database_filename", "DBASE/LAD_COIN/standard.database");
  gHcParms->Load(gHcParms->GetString("g_ctp_database_filename"), RunNumber);
  gHcParms->Load(gHcParms->GetString("g_ctp_parm_filename"));
  gHcParms->Load(gHcParms->GetString("g_ctp_kinematics_filename"), RunNumber);
  // Load params for HMS trigger configuration
  gHcParms->Load("PARAM/TRIG/thms.param");
  // Load fadc debug parameters
  gHcParms->Load("PARAM/HMS/GEN/h_fadc_debug.param");

  //===================================

  //Overwrite the existing reference times with
  //the default values specified in hallc_replay.  
  gHcParms->AddString("g_ctp_no_reference_times_filename", "PARAM/HMS/GEN/h_no_reference_times.param");
  gHcParms->Load(gHcParms->GetString("g_ctp_no_reference_times_filename"));

  //Now remove all Timing Windows and revert to 
  //the default values specifid in hallc_replay
  gHcParms->AddString("g_ctp_no_timing_windows_filename", "PARAM/HMS/GEN/hdet_cuts_no_timing_windows.param");
  gHcParms->Load(gHcParms->GetString("g_ctp_no_timing_windows_filename"));

  //===================================


  // const char* CurrentFileNamePattern = "low_curr_bcm/bcmcurrent_%d.param";
  // gHcParms->Load(Form(CurrentFileNamePattern, RunNumber));

  // Load the Hall C detector map
  gHcDetectorMap = new THcDetectorMap();
  gHcDetectorMap->Load("MAPS/HMS/DETEC/STACK/hms_stack.map");
  
  // Add trigger apparatus
  THaApparatus* TRG = new THcTrigApp("T", "TRG");
  gHaApps->Add(TRG);
  // Add trigger detector to trigger apparatus
  THcTrigDet* hms = new THcTrigDet("hms", "HMS Trigger Information");
  TRG->AddDetector(hms);

  // ROC diagnostics use the unchanged trigger decoder; tracking is unnecessary.
  auto config = new THcConfigEvtHandler("HC", "DAQ configuration");
  gHaEvtHandlers->Add(config);

  // Set up the analyzer - we use the standard one,
  // but this could be an experiment-specific one as well.
  // The Analyzer controls the reading of the data, executes
  // tests/cuts, loops over Acpparatus's and PhysicsModules,
  // and executes the output routines.
  THcAnalyzer* analyzer = new THcAnalyzer;

  // A simple event class to be output to the resulting tree.
  // Creating your own descendant of THaEvent is one way of
  // defining and controlling the output.
  THaEvent* event = new THaEvent;

  // Define the run(s) that we want to analyze.
  // We just set up one, but this could be many.
  THcRun* run = new THcRun( pathList, Form(gSystem->Getenv("REPLAY_RAW_PATTERN") ? gSystem->Getenv("REPLAY_RAW_PATTERN") : RunFileNamePattern, RunNumber) );

  // Set to read in Hall C run database parameters
  run->SetRunParamClass("THcRunParameters");
  
  // Eventually need to learn to skip over, or properly analyze the pedestal events
  run->SetEventRange(1, MaxEvent); // Physics Event number, does not include scaler or control events.
  run->SetNscan(1);
  run->SetDataRequired(0x7);
  run->Print();

  // Define the analysis parameters
  TString ROOTFileName = Form(ROOTFileNamePattern, RunNumber, MaxEvent);
  analyzer->SetCountMode(2);  // 0 = counter is # of physics triggers
                              // 1 = counter is # of all decode reads
                              // 2 = counter is event number

  analyzer->SetEvent(event);
  // Set EPICS event type
  analyzer->SetEpicsEvtType(181);
  // Define crate map
  analyzer->SetCrateMapFileName("MAPS/db_cratemap.dat");
  // Define output ROOT file
  analyzer->SetOutFile(ROOTFileName.Data());
  // Define output DEF-file 
  analyzer->SetOdefFile("CALIBRATION/set_reftimes/code/hms_reference_events.def");
  // Define cuts file
  analyzer->SetOdefFile("CALIBRATION/set_reftimes/code/hms_reference_events.def");

  // Define cuts file
  //These do not appear to effect the study on reference times.  Leaving in.



  // Start the actual analysis.
  analyzer->Process(run);

}
