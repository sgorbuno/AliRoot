// $Id$
/*
 * @file sim-hlt-tpc.C
 * @brief HLT Conformal mapping tracker embedded into AliRoot simulation.
 *
 * Example macro to run the HLT Conformal mapping tracker embedded into
 * AliRoot simulation. The reconstruction is done from the TPC digits.
 *
 * Usage: aliroot -b -q sim-hlt-tpc.C | tee sim-hlt-tpc.log
 *
 * The chain to be run is defined within the macro. The input data is
 * read from the TPC.Digits.
 *
 * The following options can be specified comma separated in a string:
 * <pre>
 *   aliroot -b -q sim-hlt-tpc.C'("options")'
 *       CA      use the cellular automaton  tracker and track merger
 *       CM      use the conformal mapping tracker and track merger
 *       SORTED  use CF pre-sorting and corresponding sequential CF
 *               algorithm, by default the algorithm capable of reading
 *               unsorted data is used
 *       RAW     write raw data for all detectors
 *       RAWHLT  write raw data only for HLT
 *       MC      propagate the MC information
 * </pre>
 *
 * The macro asumes the data to be already simulated. If it should run
 * within the initial simulation, comment the corresponding functions
 * below (SetRunGeneration etc.)
 *
 * @author Matthias.Richter@ift.uib.no
 * @ingroup alihlt_tpc
 */
sim_hlt_tpc(const char* options="CA")
{
  // this is just a tool to switch the logging systems
  AliHLTLogging log;
  //log.SwitchAliLog(0);

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  //
  // scanning the options
  //
  bool bUseCA=true;   // use the CA tracker and merger
  bool bCFSort=false; // CF pre-sorting and sequential CF algorithm
  bool bRawData=false;// raw data for all detectors
  bool bRawHLTData=false; // raw data only for HLT
  bool bPropagateMC=false;
  TString tsOptions=options;
  TObjArray* pTokens=tsOptions.Tokenize(",");
  if (pTokens) {
    for (int n=0; n<pTokens->GetEntries(); n++) {
      TString arg=((TObjString*)pTokens->At(n))->GetString();
      if (arg.CompareTo("ca", TString::kIgnoreCase)==0) {
	bUseCA=true;
      } else if (arg.CompareTo("cm", TString::kIgnoreCase)==0) {
	bUseCA=false;
      } else if (arg.CompareTo("sorted", TString::kIgnoreCase)==0) {
	bCFSort=true;
      } else if (arg.CompareTo("unsorted", TString::kIgnoreCase)==0) {
	bCFSort=false;
      } else if (arg.CompareTo("raw", TString::kIgnoreCase)==0) {
	bRawData=true;
      } else if (arg.CompareTo("rawhlt", TString::kIgnoreCase)==0) {
	bRawHLTData=true;
      } else if (arg.CompareTo("mc", TString::kIgnoreCase)==0) {
	bPropagateMC=true;
      } else {
	cout << "unknown argument: " << arg << endl;
	return 0;
      }
    }
    delete pTokens;
  }
  // summary
  cout << "using " << bUseCA?"CA":"CM" << " tracker" << endl;
  
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  //
  // init the HLT system in order to define the analysis chain below
  //
  AliHLTSystem* gHLT=AliHLTPluginBase::GetInstance();

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  //
  // define the analysis chain
  //

  // load TPCParam from OCDB
  const char* cdbEntry="TPC/Calib/Parameters";
  AliCDBManager* pMan=AliCDBManager::Instance();
  AliTPCParam* pTPCParam=NULL;
  if (pMan) {
    if (!pMan->IsDefaultStorageSet()) {
      pMan->SetDefaultStorage("local://$ALICE_ROOT/OCDB");
      pMan->SetRun(0);
    }
    AliCDBEntry *pEntry = pMan->Get(cdbEntry);
    if (pEntry && 
	pEntry->GetObject() &&
	(pTPCParam=dynamic_cast<AliTPCParam*>(pEntry->GetObject()))) {
    } else {
      HLTWarning("can not load AliTPCParam from CDB entry %s", cdbEntry);
    }
  }

  int iMinSlice=0; 
  int iMaxSlice=35;
  int iMinPart=0;
  int iMaxPart=5;
  TString mergerInput;
  TString writerInput;
  TString sinkClusterInput;
  for (int slice=iMinSlice; slice<=iMaxSlice; slice++) {
    TString trackerInput;
    for (int part=iMinPart; part<=iMaxPart; part++) {
      TString arg, publisher, cf;

      // digit publisher components
      arg.Form("-slice %d -partition %d", slice, part);
      publisher.Form("DP_%02d_%d", slice, part);
      AliHLTConfiguration pubconf(publisher.Data(), "TPCDigitPublisher", NULL , arg.Data());

      // cluster finder components
      arg="-timebins ";
      if (pTPCParam) arg+=pTPCParam->GetMaxTBin()+1;
      else arg+=446; // old simulated data
      if (bCFSort) arg+=" -sorted ";
      if (bPropagateMC) arg+=" -do-mc ";
      cf.Form("CF_%02d_%d", slice, part);
      AliHLTConfiguration cfconf(cf.Data(), "TPCClusterFinderUnpacked", publisher.Data(), arg.Data());
      if (trackerInput.Length()>0) trackerInput+=" ";
      trackerInput+=cf;
      if (sinkClusterInput.Length()>0) sinkClusterInput+=" ";
      sinkClusterInput+=cf;
    }

    TString tracker;
    // tracker finder components
    tracker.Form("TR_%02d", slice);
    if (bUseCA) {
      AliHLTConfiguration trackerconf(tracker.Data(), "TPCCATracker", trackerInput.Data(), "");
    } else {
      AliHLTConfiguration trackerconf(tracker.Data(), "TPCSliceTracker", trackerInput.Data(), "-pp-run");
    }

    //add all trackers to writer input. Include if you would like all slice tracks written.
    //if (writerInput.Length()>0) writerInput+=" ";
    //writerInput+=tracker;

    // add all clusterfinders to the writer input
    if (writerInput.Length()>0) writerInput+=" ";
    writerInput+=trackerInput;

    if (mergerInput.Length()>0) mergerInput+=" ";
    mergerInput+=tracker;

  }

  // GlobalMerger component
  if (bUseCA) {
    AliHLTConfiguration mergerconf("globalmerger","TPCCAGlobalMerger",mergerInput.Data(),"");
  } else {
    AliHLTConfiguration mergerconf("globalmerger","TPCGlobalMerger",mergerInput.Data(),"");
  }

  TString converterInput="globalmerger";

  // collector for the MC information to be propagated from CFs to ESDConverter
  if (bPropagateMC){
    AliHLTConfiguration mcinfo("mcinfo", "BlockFilter"   , sinkClusterInput.Data(), "-datatype 'CLMCINFO' 'TPC '");  
    AliHLTConfiguration mcTrackMarker("mcTrackMarker","TPCTrackMCMarker","globalmerger mcinfo","" );
    converterInput+=" mcTrackMarker";
  }

  if (writerInput.Length()>0) writerInput+=" ";
  writerInput+="globalmerger";


  //////////////////////////////////////////////////////////////////////////////////////////
  //
  // output section
  //

  //////////////////////////////////////////////////////////////////////////////////////////
  // sink1: id=sink-writeall write all blocks
  AliHLTConfiguration sink1("sink-writeall", "FileWriter"   , writerInput.Data(), "-specfmt -subdir=event_%d -blcknofmt=_0x%x -idfmt=_0x%08x");


  //////////////////////////////////////////////////////////////////////////////////////////
  // sink2: id=sink-esd-file write ESD using the TPCEsdWriter
  AliHLTConfiguration sink2("sink-esd-file", "TPCEsdWriter"   , converterInput.Data(), "-datafile AliHLTESDs.root");


  //////////////////////////////////////////////////////////////////////////////////////////
  // sink3: id=sink-esd add ESD to HLTOUT using the TPCEsdConverter

  // the esd converter configuration
  AliHLTConfiguration sink3("sink-esd", "TPCEsdConverter"   , converterInput.Data(), "");

  //////////////////////////////////////////////////////////////////////////////////////////
  // sink4: id=sink-clusters add cluster data blocks to HLTOUT
  AliHLTConfiguration sink4("sink-clusters", "BlockFilter"   , sinkClusterInput.Data(), "-datatype 'CLUSTERS' 'TPC '");

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  //
  // Init and run the HLT simulation
  // All but HLT simulation is switched off
  //
  AliSimulation sim;
 
  // switch of simulation and data generation
  // comment all that stuff to also simulate the events and data
  sim.SetRunGeneration(kFALSE);
  sim.SetMakeDigits("");
  sim.SetMakeSDigits("");
  sim.SetMakeDigitsFromHits("");
  //sim.SetMakeTrigger("");
  sim.SetRunQA(":");

  // the normal simulation sets the specific storage for the GRP entry
  if (gSystem->AccessPathName("GRP/GRP/Data")) {
    cerr << "*********************************************************" << endl;
    cerr << "error: no GRP entry found in the currect directory, simulation might be incomplete. Skip setting specific storage for GRP entry" << endl;
    cerr << "*********************************************************" << endl << endl;
  } else {
    sim.SetSpecificStorage("GRP/GRP/Data",
			   Form("local://%s",gSystem->pwd()));
  }

  TString rawDataSelection="HLT";
  if (bRawData) rawDataSelection="ALL";
  if (bRawHLTData || bRawData) sim.SetWriteRawData(rawDataSelection, "raw.root", kTRUE);

  // set the options for the HLT simulation
  sim.SetRunHLT("libAliHLTUtil.so libAliHLTTPC.so loglevel=0x7c "
		"chains=sink-esd,sink-clusters");
  sim.Run();
}
