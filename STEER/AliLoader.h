#ifndef ALIGETTER_H
#define ALIGETTER_H


#include <AliConfig.h>
class TString;
class TFile;
class TFolder;
class TTree;
class TTask;
class AliRunLoader;
class AliDigitizer;


//this was supposed to be struct, but Root I/O does not support C structures
//thus it is looking like this
class AliLoaderDataInfo
 {
  public:
   AliLoaderDataInfo(){fFile=0x0;fDirectory=0x0;}
   virtual ~AliLoaderDataInfo(){}
   
   TString      fFileName; //name of the file 
   TFile*       fFile; //! pointer to file 
   TDirectory*  fDirectory; //!pointer to TDirectory
   TString      fFileOption; //!file option while opened 
   TString      fContainerName;//Name of the container (usually tree)
   TString      fName;
   ClassDef(AliLoaderDataInfo,1)
 };

class AliLoader: public TNamed
 {
//base class for loaders
//loaders is common for reading data for all detectors
//Each detector has a getter data member
//getter is accessible via folder structure as well

   public:
    AliLoader();
    AliLoader(const Char_t *detname,const Char_t *eventfoldername); //contructor with name of the top folder of the tree
    AliLoader(const Char_t *detname,TFolder* eventfolder);

    virtual ~AliLoader();//----------------- 
 
    virtual Int_t  GetEvent();//changes to root directory
                             //relies on the event number defined in gAlice
    virtual Int_t  SetEvent();//basically the same that GetEvent but do not post data to folders
    virtual void   MakeTree(Option_t* opt);

    //these methods are responsible for reading data
    // 1. Opens the file (accordingly to gAlice->GetEvNumber()/Nevents per File
    // 2. Changes to proper ROOT directory
    // 3. Retrives data and posts to folders
    //options: 
    //   ""       - readonly file
    //   "update" - update file
    //   "recreate" 
    Int_t          LoadHits(Option_t* opt=""){return LoadData(kHits,opt);}
    Int_t          LoadSDigits(Option_t* opt=""){return LoadData(kSDigits,opt);}
    Int_t          LoadDigits(Option_t* opt=""){return LoadData(kDigits,opt);}
    Int_t          LoadRecPoints(Option_t* opt=""){return LoadData(kRecPoints,opt);}
    Int_t          LoadTracks(Option_t* opt=""){return LoadData(kTracks,opt);}
    
    Int_t          LoadSDigitizer(Option_t* opt="");
    Int_t          LoadDigitizer(Option_t* opt="");
    Int_t          LoadReconstructioner(Option_t* opt="");
    Int_t          LoadTracker(Option_t* opt="");

    void           UnloadHits(){CleanHits();CloseHitsFile();}
    void           UnloadSDigits(){CleanSDigits();CloseSDigitsFile();}
    void           UnloadDigits(){CleanDigits();CloseDigitsFile();}
    void           UnloadRecPoints(){CleanRecPoints();CloseRecPointsFile();}
    void           UnloadTracks(){CleanTracks();CloseTracksFile();}

    virtual Int_t  ReloadHits();  //unload and load again Hits
    virtual Int_t  ReloadSDigits(); //unload and load again 
    virtual Int_t  ReloadDigits(); //unload and load again 
    virtual Int_t  ReloadRecPoints(); //unload and load again 
    virtual Int_t  ReloadTracks(); //unload and load again 
    virtual Int_t  ReloadAll(); //unload and load again everything that was loaded 
    
  //these methods writes object from folder to proper file
    virtual Int_t  WriteHits(Option_t* opt=""){return WriteData(kHits,opt);}
    virtual Int_t  WriteSDigits(Option_t* opt=""){return WriteData(kSDigits,opt);}
    virtual Int_t  WriteDigits(Option_t* opt=""){return WriteData(kDigits,opt);}
    virtual Int_t  WriteRecPoints(Option_t* opt=""){return WriteData(kRecPoints,opt);}
    virtual Int_t  WriteTracks(Option_t* opt=""){return WriteData(kTracks,opt);}
    virtual void   Write(Option_t* opt="all"){}
    
    virtual Int_t  WriteSDigitizer(Option_t* opt="");
    virtual Int_t  WriteDigitizer(Option_t* opt="");
    virtual Int_t  WriteReconstructioner(Option_t* opt="");
    virtual Int_t  WriteTracker(Option_t* opt="");
    
//   virtual Int_t  WritePIDer(Option_t* opt="");
    
  //void SetTopFolder(TString& str){}; //Sets root top folder for the run
    
    void           SetHitsFileName(const TString& fname){FileName(kHits) = fname;}
    void           SetSDigitsFileName(const TString& fname){FileName(kSDigits) = fname;}
    void           SetDigitsFileName(const TString& fname){FileName(kDigits) = fname;}
    void           SetRecPointsFileName(const TString& fname){FileName(kRecPoints) = fname;}
    void           SetTracksFileName(const TString& fname){FileName(kTracks) = fname;}

    const TString& GetHitsFileName(){return FileName(kHits);}
    const TString& GetSDigitsFileName(){return FileName(kSDigits);}
    const TString& GetDigitsFileName(){return FileName(kDigits);}
    const TString& GetRecPointsFileName(){return FileName(kRecPoints);}
    const TString& GetTracksFileName(){return FileName(kTracks);}
  
    Int_t          SetEventFolder(TFolder* eventfolder);//sets the event folder
    Int_t          Register();//triggers creation of subfolders for a given detector
      
    TFolder*       GetTopFolder();  //returns top aliroot folder
    TFolder*       GetEventFolder();//returns the folder that event is sitting
    TFolder*       GetDataFolder(); //returns the folder that hits, sdigits, etc are sitting (this contains folders named like detectors)
    TFolder*       GetDetectorDataFolder();//returns the folder that hits, sdigits, etc 
                                     //are sitting for a given detector (subfolder of Data Folder)
    TFolder*       GetModulesFolder();
    TFolder*       GetTasksFolder();
    TFolder*       GetQAFolder();
    
    TTask*         SDigitizer();//return detector SDigitizer()
    AliDigitizer*  Digitizer();
    TTask*         Reconstructioner();
    TTask*         Tracker();
    TTask*         QAtask(const char* name = 0x0);

    TObject**      SDigitizerRef();
    TObject**      DigitizerRef();
    TObject**      ReconstructionerRef();
    TObject**      TrackerRef();
    
    virtual void   MakeHitsContainer();
    virtual void   MakeSDigitsContainer();
    virtual void   MakeDigitsContainer();
    virtual void   MakeRecPointsContainer();
    virtual void   MakeTracksContainer();
        
    const TString& GetHitsContainerName() const { return fDataInfo[kHits].fContainerName;}
    const TString& GetSDigitsContainerName() const { return fDataInfo[kSDigits].fContainerName;}
    const TString& GetDigitsContainerName() const { return fDataInfo[kDigits].fContainerName;}
    const TString& GetRecPointsContainerName() const { return fDataInfo[kRecPoints].fContainerName;}
    const TString& GetTracksContainerName() const { return fDataInfo[kTracks].fContainerName;}

    virtual void   CleanFolders();
    virtual void   CloseFiles();
    
    TTree*         TreeH(); //returns the tree from folder; shortcut method
    TTree*         TreeS(); //returns the tree from folder; shortcut method
    TTree*         TreeD(); //returns the tree from folder; shortcut method
    TTree*         TreeR(); //returns the tree from folder; shortcut method
    TTree*         TreeT(); //returns the tree from folder; shortcut method

    virtual Int_t PostSDigitizer(TTask* sdzer);//adds it to Run SDigitizer
    virtual Int_t PostDigitizer(AliDigitizer* task);
    virtual Int_t PostReconstructioner(TTask* task);
    virtual Int_t PostTracker(TTask* task);
    
    virtual void  CleanHits()     {Clean(kHits);}//cleans hits from folder
    virtual void  CleanSDigits()  {Clean(kSDigits);}
    virtual void  CleanDigits()   {Clean(kDigits);}
    virtual void  CleanRecPoints(){Clean(kRecPoints);}
    virtual void  CleanTracks()   {Clean(kTracks);}
    
    virtual void  CleanSDigitizer();
    virtual void  CleanDigitizer();
    virtual void  CleanReconstructioner();
    virtual void  CleanTracker();

    void          SetHitsFileOption(Option_t* newopt);//Hits File Option in open
    void          SetSDigitsFileOption(Option_t* newopt);//
    void          SetDigitsFileOption(Option_t* newopt);//
    void          SetRecPointsFileOption(Option_t* newopt);//
    void          SetTracksFileOption(Option_t* newopt);//
    
    virtual void  SetCompressionLevel(Int_t cl);//Sets compression level in all the files
    void          SetDirName(TString& name);//sets the directory name for all the I/O environment
    
    const TString& GetDetectorName() const{return fDetectorName;}//returns the name of the detector
    AliRunLoader*  GetRunLoader();//gets the run-getter from event folder
    
    void          SetDigitsFileNameSuffix(const TString& suffix);//adds the suffix before ".root", 
                                                          //e.g. TPC.Digits.root -> TPC.DigitsMerged.root
                                                              //made on Jiri Chudoba demand
    
    
   protected:

    /*********************************************/
    /************    PROTECTED      **************/
    /*********     M E T H O D S       ***********/
    /*********************************************/
    enum EDataTypes{kHits,kSDigits,kDigits,kRecPoints,kTracks,kNDataTypes};

    Int_t         LoadData(EDataTypes dt,Option_t* opt);
    Int_t         PostData(EDataTypes dt);
    Int_t         WriteData(EDataTypes dt,Option_t* opt);
    Int_t         ReloadData(EDataTypes dt);
    void          UnloadData(EDataTypes dt){Clean(dt);CloseDataFile(dt);}
    //Opens hits file and jumps to directory cooresponding to current event.
    //If dir does not exists try to create it
    //opt is passed to TFile::Open
    Int_t         OpenHitsFile(Option_t* opt){return OpenDataFile(kHits,opt);}
    Int_t         OpenSDigitsFile(Option_t* opt){return OpenDataFile(kSDigits,opt);}
    Int_t         OpenDigitsFile(Option_t* opt){return OpenDataFile(kDigits,opt);}
    Int_t         OpenRecPointsFile(Option_t* opt){return OpenDataFile(kRecPoints,opt);}
    Int_t         OpenTracksFile(Option_t* opt){return OpenDataFile(kTracks,opt);}
    Int_t         OpenDataFile(const TString& filename,TFile*& file,TDirectory*& dir,Option_t* opt);
    Int_t         OpenDataFile(EDataTypes dt,Option_t* opt)
                    {return OpenDataFile(SetFileOffset(FileName(dt)),File(dt),Directory(dt),opt);}

    void          CloseHitsFile(){CloseDataFile(kHits);}
    void          CloseSDigitsFile(){CloseDataFile(kSDigits);}
    void          CloseDigitsFile(){CloseDataFile(kDigits);} 
    void          CloseRecPointsFile(){CloseDataFile(kRecPoints);}
    void          CloseTracksFile(){CloseDataFile(kTracks);}
    void          CloseDataFile(TFile*& file,TDirectory*& dir);
    void          CloseDataFile(EDataTypes dt){CloseDataFile(File(dt),Directory(dt));}
    //reads data from the file and posts them into folder
    virtual Int_t PostHits(){return PostData(kHits);} 
    virtual Int_t PostSDigits(){return PostData(kSDigits);} 
    virtual Int_t PostDigits(){return PostData(kDigits);} 
    virtual Int_t PostRecPoints(){return PostData(kRecPoints);} 
    virtual Int_t PostTracks(){return PostData(kTracks);} 

    virtual Int_t PostSDigitizer();//gets SDigitizer from file and adds it to Run SDigitizer
    virtual Int_t PostDigitizer();//gets Digitizer from file and adds it to Run Digitizer
    virtual Int_t PostReconstructioner();//gets Reconstructioner from file and adds it to Run Reconstructioner
    virtual Int_t PostTracker();//gets tracker from file and adds it to Run Tracker
    
    void          Clean(const TString& name);
    void          Clean(EDataTypes dt){Clean(ContainerName(dt));}
    TTree*        Tree(EDataTypes dt);
    void          MakeTree(EDataTypes dt);
    void          SetFileOption(EDataTypes dt,Option_t* newopt);

    TString       GetUnixDir();
    TObject*      GetDetectorData(const char* name){return GetDetectorDataFolder()->FindObject(name);}
    TObject**     GetDetectorDataRef(TObject* obj);
    
    Bool_t        CheckReload(const TFile* file, const TString& basefilename);//checks if we have to reload given file
    void InitDefaults();
    void ResetDataInfo();

    TFile*&       File(EDataTypes dt){return fDataInfo[dt].fFile;}
    TDirectory*&  Directory(EDataTypes dt){return fDataInfo[dt].fDirectory;}
    TString&      FileName(EDataTypes dt){return fDataInfo[dt].fFileName;}
    TString&      ContainerName(EDataTypes dt){return fDataInfo[dt].fContainerName;}
    TString&      FileOption(EDataTypes dt){return fDataInfo[dt].fFileOption;}
    TString&      DataName(EDataTypes dt){return fDataInfo[dt].fName;}
    


    /**********************************************/
    /************    PROTECTED      ***************/
    /*********        D A T A          ************/
    /**********************************************/

    TString       fDetectorName;//detector name that this loader belongs to
    
    AliLoaderDataInfo fDataInfo[kNDataTypes]; // array with data type information (file name, pointer to file...)

//these variables were introduced because if TFile is created with "recreated" stored option is "CREATE"
//and we need to remeber this (Max events per file functionality)

    TFolder*      fEventFolder; //! Folder with data that changes from event to event, even in single run
    TFolder*      fDataFolder;  //!  Folder with data (hits, sdigits, etc, grouped in folders named like detectors
    TFolder*      fDetectorDataFolder;//!Folder that contains the detector data
    TFolder*      fModuleFolder;      //!Folder that contains the modules 

    TFolder*      fTasksFolder;       //!Folder that contains the Tasks (sdigitizer, digitizer, reconstructioner)
    TFolder*      fQAFolder;          //!Folder that contains the QA objects
    
    static const TString   fgkDefaultHitsContainerName;//default name of conatiner (TREE) for hits
    static const TString   fgkDefaultDigitsContainerName;//default name of conatiner (TREE) for digits
    static const TString   fgkDefaultSDigitsContainerName;//default name of conatiner (TREE) for Sdigits
    static const TString   fgkDefaultRecPointsContainerName;//default name of conatiner (TREE) for Rec Points
    static const TString   fgkDefaultTracksContainerName;//default name of conatiner (TREE) for Tracks
    
    
    static const TString   fgkLoaderBaseName;//base name of Loader: fDetectorName+fgkLoaderBaseName. i.e. PHOSLoader
    
   private:
    //descendant classes should
    //use protected interface methods to access these folders
    

    /**********************************************/
    /***********     P U B L I C     **************/
    /*********       S T A T I C       ************/
    /*********         METHODS         ************/
    /*********     They are used by    ************/
    /*********** AliRunLoader as well**************/
    /**********************************************/
   public:
    static TDirectory*    ChangeDir(TFile* file, Int_t eventno); //changes the root directory in "file" to directory corresponing to eventno
    static Bool_t         TestFileOption(Option_t* opt);//checks is file is created from scratch
    const TString SetFileOffset(const TString& fname);//adds the proper number before .root exttension suffix

    ClassDef(AliLoader,1)
 };
/******************************************************************/
/************************ I N L I N E S ***************************/
/******************************************************************/

inline TFolder* AliLoader::GetDetectorDataFolder()
 {
 //helper function which returns the folder of name "name" in data folder
   if(!fDetectorDataFolder)
    {
      fDetectorDataFolder = dynamic_cast<TFolder*>(GetDataFolder()->FindObject(fDetectorName.Data()));
      if(!fDetectorDataFolder)
       {
         Fatal("GetDetectorDataFolder",
               "Can not find folder %s in folder %s. Aborting",
               fDetectorName.Data(),GetDataFolder()->GetName());
         return 0x0;
       }
      
    }
   return fDetectorDataFolder;
 }

/******************************************************************/
/******************************************************************/
/******************************************************************/
/************************ DATA INFO ***************************/
/******************************************************************/
/******************************************************************/
/******************************************************************/

#endif
