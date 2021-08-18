/*
################################################################################
#
#  egs_brachy_GUI trim.h
#  Copyright (C) 2021 Shannon Jarvis, Martin Martinov, and Rowan Thomson
#
#  This file is part of egs_brachy_GUI
#
#  egs_brachy_GUI is free software: you can redistribute it and/or modify it
#  under the terms of the GNU Affero General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  egs_brachy_GUI is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Affero General Public License for more details:
#  <http://www.gnu.org/licenses/>.
#
################################################################################
#
#  When egs_brachy is used for publications, please cite our paper:
#  M. J. P. Chamberland, R. E. P. Taylor, D. W. O. Rogers, and R. M.Thomson,
#  egs brachy: a versatile and fast Monte Carlo code for brachytherapy, 
#  Phys. Med. Biol. 61, 8214-8231 (2016).
#
#  When egs_brachy_GUI is used for publications, please cite our paper:
#  To Be Announced
#
################################################################################
#
#  Author:        Shannon Jarvis
#
#  Contributors:  Martin Martinov
#                 Rowan Thomson
#
################################################################################
*/

#ifndef PARSE_DICOM_H
#define PARSE_DICOM_H

#include <QtGui>
#include <QtWidgets>
#include <iostream>
#include <math.h>
#include <egsphant.h>

#include "options.h"
#include "egsinp.h"
#include "file_selector.h"
#include "tissue_check.h"
#include "egsinp.h"
#include "priority.h"
#include "read_dose.h"
#include "metrics.h"
#include "preview.h"
#include "trim.h"

struct cont {
    double x;
    double y;
    double z;
};

// These need to be declared ahead of time, they are needed for nested sequences
class Sequence;
class SequenceItem;
class Attribute;
class DICOM;
// The following two classes are used to hold a sequence of items (and yes, you
// can have nested sequences, cause, you know, why not?)
class Sequence {
public:
    QVector <SequenceItem *> items;
    ~Sequence();
};

class SequenceItem {
public:
    unsigned long int vl; // Value Length
    unsigned char *vf; // Value Field
    Sequence seq; // Contains potential sequences

    SequenceItem(unsigned long int size, unsigned char *data);
    SequenceItem(unsigned long int size, Attribute *data);
    ~SequenceItem();
};

// Might as well be a struct, but I might want some methods in the future
class Attribute {
public:
    unsigned short int tag[2]; // Element Identifier
    QString desc; // Desciption
    unsigned short int vr; // Value Representation
    unsigned long int vl; // Value Length
    unsigned char *vf; // Value Field
    Sequence seq; // Contains potential sequences

    Attribute();
    ~Attribute();
};

// These are all defined in database.cpp so as to save alot of recompiling
// hassle
struct Reference {
    unsigned short int tag[2]; // Element Identifier
    QString vr; // Element Identifier
    QString title; // Title of element
};

class database : public QObject {
    Q_OBJECT

public:
    // Contains a list of known attribute entries
    QVector <Reference *> lib;
    // Contains all the acceptable value representations
    QStringList validVR;
    QStringList implicitVR;

    database();
    ~database();

    Reference binSearch(unsigned short int one, unsigned short int two, int min,
                        int max);
};

class DICOM : public QWidget {
    Q_OBJECT

signals:
    void progressMade(double n); // Update the progress bar

public slots:
    void updateProgress(double n);      //Updates the progress bar

public:
    // Contains all the data read in from a dicom file sorted into attributes
    QVector <Attribute *> data;

    // Pointer to precompiled DICOM library
    database *lib;

    // Transfer syntax
    bool isImplicit, isBigEndian;
    bool loadedct = false;
    bool loadedplan = false;
    bool loadedstruct = false;
    bool externalStruct = false;
    // z height (default to NaN, only change if slice height tag is found)
    double z = std::nan("1");

    // file location for later lookup
    QString path;

    DICOM(database *);
    DICOM();
    ~DICOM();

    int parse(QString p);
    int readSequence(QDataStream *in, Attribute *att);
    int readDefinedSequence(QDataStream *in, Attribute *att, unsigned long int n = 0);

    int parseSequence(QDataStream *in, QVector <Attribute *> *att);

    void extract(QVector<QString> tempS2);
    void extract_data_for_dicomdose(DICOM *dicom);

    void submerge(QVector <DICOM *> &data, int i, int c, int f);
    void mergeSort(QVector <DICOM *> &data, int n);

    void create_3ddose(QString file_path, QString path_3ddose);

    int get_idx_from_bounds(double pos, QVector<double> bounds);
    QVector<int> get_ijk_from_xyz(double x, double y, double z);


    // PROGRESS BAR~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void setup_progress_bar_dicom(QString window_title, QString text);  //Creates the progress bar
    void create_progress_bar_dicom();
    void destruct_progress_bar_dicom();

    double *remainder2;
    QWidget *progWin2;
    QGridLayout *progLayout2;
    QProgressBar *progress2;
    QLabel *progLabel2;

    QMap <QString, QString> dicomHeader;

    // For CT data ---------------------------------- //
    double rescaleM = 1, rescaleB = 0, rescaleFlag;
    QVector <QVector <QVector <short int> > > HU;
    QVector <unsigned short int> xPix;
    QVector <unsigned short int> yPix;
    int numZ;
    QVector <QVector <double> > imagePos;
    QVector <QVector <double> > xySpacing;
    QVector <double> zSpacing;

    // For RS Struct data ---------------------------------- //
    QVector <QVector <QPolygonF> > structPos;
    QVector <QVector <double> > structZ;
    QVector<QVector<QVector<double>>> extrema;
    QVector <QString> structName;
    QVector <int> structNum;
    QMap <int, int> structLookup;
    QVector <int> structReference;
    QVector<QString> structType;
    QVector<int> indexExternal;

    // For RP Plan data ---------------------------------- //
    QString treatment_type;
    QString treatment_technique;
    QString isotope_name;           //seed name determined for the isotope name in the dicom file
    double air_kerma;
    QVector<QString> Seed_info; //Infromation from private tags about the seed


    double DSF_intermediate; //need air kerma per history (from seed) to calculate
    QVector<QVector<coordinate>> all_seed_pos; //coordinate of each seed
    QVector<QVector<double>> all_seed_time; //coordinate of each seed
    double total_time = 0;


};



class Interface : public QWidget {
private:
    Q_OBJECT // This line is necessary to create custom SLOTs, ie, functions
    // that define what happens when you click on buttons

    void setup_default_files();
    void continue_AT_egsinp();
    bool checkDefaultFile(QString filepath);
    bool get_seed_from_user();          //Pop-up window to ask user to verity/select the brachytherapy seed
    bool select_air_kerma(QString seed);
    bool readCalib();
    QVector<QString> get_seedList();    //Retrieve list of possible seeds from egs_brachy (EGSnrc_with_egs_brachy/egs_home/egs_brachy/lib/geometry/sources/)

    QString getPath(QString path);  //Returns the path of an alias
    QString density; //HU to density file


    //Locations of default files
    bool tg43Flag = false;              //Flag if TG-43 simulation is being done
    bool AT_tg43Flag = false;
    bool extraOptionsFlag = false;     //Flag to indicate if extra options or the 'main' tab is running
    bool createEgsinp = true;
    QString TAS_file;                   //Location of tissue assignment scheme file
    QString AT_TAS_file;

    QVector<QString> source_element_folder_names;
    QVector<QString> privateSeedInfo;
    int PredictedSeedIndex = 0;
    double air_kerma_per_history = 1.0;     //Value obtained from the selected seed .Sk file
    bool air_kerma_per_history_found = false;
    double DSF;                         //Calculated DSF (DSF_intermediate/air_kerma_per_history)

    //Path of default files/ files used for esginp files
    QVector<QString> spectrum_files;
    QString material_data_file;
    QString muen_file_substring;
    QString low_energy_transport_file;
    QString high_energy_transport_file;

    //Variables and funcions used for reading the tissue assignment scheme
    bool read_TAS_file(QString file_path, bool containsExternal);
    QVector<QString> structure;
    QVector<QString> tissue;
    QVector<double> density_max;

    void create_egsinp_files();
    void egs_from_phantom();    //sets up the create dose from phantom file
    void setup_egsphant();

    QString element;    //element folder containing the user-verified seed
    QString seed;       //user-verified seed
    QString isotope;
    QVector<QString> media;     //all tissues in the phantom
    QVector<bool> genMask;

    void create_egsphant_all_voxels_default_tissue();
    void continue_create_egsphant_all_voxels_default_tissue();

    bool tas_contains_default;
    bool tas_contains_default_ext;
    bool tas_contains_default_int;

    QString trimExisitngEGS_Mid(QString phant_file);

    QVector <double> denThreshold,denThreshold_int;
    QVector <QString> medThreshold, medThreshold_int;
    QVector <QVector <QString> > medThresholds;
    QVector <QVector <double> > denThresholds;
    QVector <bool> structUnique;
    QVector <bool> external;
    QVector <int> structPrio;

    QVector <double> HUMap, denMap;
    bool readCalibFile = false;
    QMap <QString,unsigned char> mediaMap;

    //Finctions and veriables for MAR/STR
    QVector<int> get_ijk_from_xyz(double x, double y, double z); // returns the (i,j,k) of the dose dist
    int get_idx_from_bounds(double pos, QVector<double> bounds);    //returns the 1 dimensional index of the voxel whose bounds contain pos
    bool inPhantom(double x, double y, double z);

    //Default values for STR
    bool setup_MAR_Flag  = false;
    double high_threshold=1.16;         //threshold density value
    double low_threshold = 0.800;
    double replacement=0.917;       //replacement density value
    double xy_search_in_mm=5;


    QVector<double> get_voxel_center_from_ijk(double ii, double jj, double kk);

    EGSPhant phant;
    QVector <EGSPhant *> masks;

public:
    Interface();
    ~Interface();

    void setup_progress_bar(QString window_title, QString text);    //Creates the progress bar
    double interp(double x, double x1, double x2, double y1, double y2);


public slots:
    // All the function calls when something is pressed

    void load_dicom();                  //User selects directory containing DICOM files, the files are read and necessary information is extracted
    void loadCalib();                   //Loads and reads the calibration curve file
    void check_dicom_inputs();          //To initiate creating the egs input files
    void updateProgress(double n);      //Updates the progress bar
    void show_advanced_options();       //Pop-up window to change default egsinp parameters
    void show_trim_options();
    void change_file_location();        //Creates window where user can select the location of the egsinp, egsphnat nad transformation files
    void create_egsphant();         //Creates all the files required for egs_brachy
    void ATcreate_egsphant();       //Creates the egsphant for the additional tab/extra options tab
    void set_tissue_selection();

    //void egs_from_phantom();          //all the phantoms, transformation file and transport parameters in egs_brachy
    void select_tas_file();
    void select_phantom_file();
    void select_transform_file();
    void select_transport_file();
    void change_phantom_file_location();
    void load_muen();
    void load_material();
    void run_brachy_phantom();
    //void continue_create_input_files();
    void closeWindow(); // This is called to when close1 to close4 are pressed
    void readError(); // This is the same as readOutput () but with errors
    // instead
    void reenableButtons(); // Since lets you make a new simulation after the
    // one is done
    void reenableButtons_phantom(); //copies reenableButtons for use with the phantom tab
    void closed_metrics();
    void read_3ddose_output_metrics();
    void launch_dose_to_3ddose();
    void read_3ddose();
    void readOutput(); // This is used by all QProcesses and adds output to the
    // console window

    QString stat(double min, double max, double nBin, QVector<double> diff);

    void show_launch_egs();             //Calls egs_brachy for the idocm tab
    void show_launch_egs_phantom();     //Calls egs_brachy for the phantom tab

    void stop_process();

    void loadDICOMfile();
    void setup_input_files();
    void create_input_files();

    //MAR/STR slots
    void apply_str_to_seed_locations(); //applies STR

    // void continue_egsphant();
    void setup_MAR();   //calls STR if treatment type == LDR

    void delete_preview();
    void show_preview();

    void trimExisitngEGS();
    void trimPhant_notAlreadyExisting();

    void checkTG43simul();
    void checkTG43();

    void enable_midTab_trim(int row);
    void showTrim_midtab();

public:

// LAYOUT~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    QWidget *window;
    QGridLayout *mainLayout;
    QPushButton *close;
    QPushButton *run;
    QVBoxLayout *col_1;
    QVBoxLayout *col_2;


    QPushButton *PreviewButton;
    QPushButton *launchButton;
    QPushButton *options_button;
    QPushButton *trim_button;
    QPushButton *save_file_location_button;

    QPushButton *change_muen;
    QPushButton *change_material_file;
    QPushButton *save_file_location_phantom;

    QPushButton *three_ddose_to_dose;
    QPushButton *create_dose_to_3ddose;
    QPushButton *compare_3ddose;
    QPushButton *dose_metrics;

    QGroupBox *ioFrame;
    QGroupBox *input;
    QPushButton *DICOMButton;
    QPushButton *CTButton;
    QPushButton *CalibButton;
    QGroupBox *modularFrame;
    QGridLayout *modularLayout;

    QPushButton *create_input_files_button;

    QLineEdit *numb_histories;
    QLineEdit *numb_batch;
    QLineEdit *numb_chunk;

    QGroupBox *fromdicom;
    QGroupBox *from3ddose;
    QGridLayout *fromdicomLayout;
    QGridLayout *from3ddoseLayout;

    QGridLayout *ioLayout;
    QGridLayout *ouLayout;

    bool selected_tas = false;
    bool close_console = true;                  //flag to indicate if the console close button has changed the text

    QWidget *selectPhant;
    QPushButton *no_phant_selection;
    QPushButton *selected_phant;
    QListWidget *phant_list;

    // QRadioButton *typeBrst;
    // QRadioButton *typePep;
    // QRadioButton *typePros;
    // QRadioButton *type43;
    // QRadioButton *typeOther;
    // QRadioButton *typeEye;
    QListWidget *tas_list;
    QPushButton *select_tas_bttn;
    QRadioButton *tg43_simul_check;

    QListWidget *tas_selection;
    QPushButton *select_tas;
    QRadioButton *tg43_simul;

    // QRadioButton *typeBrst1;
    // QRadioButton *typeEye1;
    // QRadioButton *typePep1;
    // QRadioButton *typePros1;
    QRadioButton *type431;
    // QRadioButton *typeOther1;

    QVector<QString> list_of_contour_tas;
    QPushButton *calibrationButton;

    QGroupBox *tas_grid;
    QGroupBox *phantBox_inputfiles;
    QListWidget *phant_selection_inputfiles;
    QPushButton *select_phantom_inputfiles;
    QPushButton *trim_phantom_inputfiles;
    QListWidget *seed_selection_inputfiles;
    QGroupBox *seedBox_inputfiles;
    QPushButton *select_seed_inputfiles;
    QGroupBox *grid;
    QGridLayout *vbox;
    QVBoxLayout *option;
    QGroupBox *opt_button;
    QGridLayout *outLayout;
    QGroupBox *outFrame;

    QGridLayout *optionsLayout;
    QGroupBox *optionsFrame;

    QStringList tas_names;
    QStringList tas_names_path;
    QStringList seed_names;
    QStringList seed_names_path;
    QStringList phant_names;
    QStringList phant_names_path;
    QString selected_muen_file;
    QString selected_material_file;
    QStringList transport_names;
    QStringList transport_names_path;

    QRadioButton *normal;
    QRadioButton *superposition;
    QRadioButton *volume_correction;
    QRadioButton *yes;
    QRadioButton *no;
    //QGroupBox *egsBox;
    QGroupBox *energyDepos;
    QGroupBox *numbHist;
    QGroupBox *runMode;
    QPushButton *launch_phantom; //Create input files and launch egs_brachy


    QListWidget *phant_selection;
    QListWidget *seed_selection;
    QListWidget *transport_selection;
    QPushButton *select_transformation_file;
    QPushButton *select_phantom;
    QPushButton *select_transport;
    QGroupBox *phantBox;
    QGroupBox *seedBox;
    QGroupBox *transportBox;

    QString working_path;
    bool read_error; //flag to identify if read error with console

// PROGRESS BAR~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    double *remainder;
    QWidget *progWin;
    QGridLayout *progLayout;
    QProgressBar *progress;
    QLabel *progLabel;

// Launching egs ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    QWidget *message;

    // Console
    QTextEdit *console; // This holds output from all the execute functions
    QPushButton *close1; // This closes the console
    QPushButton *kill; // This stops processes
    QPushButton *metrics_button;
    QGridLayout *conLayout;
    QWidget *conWin;
    bool *batch;

    // BrachyDose Run Options
    QErrorMessage *errors; // This is the BrachyDose error message
    QGridLayout *runLayout;
    QWidget *runBrachy; // This is the running BrachyDose window
    QPushButton *runInter;
    // LineInput *numBatch;
    QPushButton *runBatch;
    int *tempNumBatch;
    QPushButton *close4; // This closes the run BrachyDose window

    QProcess *brachy;

    QString egs_brachy_home_path;
    QString source_folder_path;

    DICOM *get_data;
    priority *get_prio;
    DICOM *get_some_data;
    egsinp *egs_input;
    egsinp *egs_input_phantom;
    egsinp *egs_input_file;
    Options *options;
    Preview *preview;
    File_selector *select_location;
    File_selector *select_location_phantom;
    File_selector *select_location_additional_function;
    Tissue_check *tissue_check;
    trim *trimEGS;
    trim *trimEGSMid;

    read_dose *dose;
    metrics *calc_metrics;
    metrics *calc_metrics_3ddose;

    // Declaring tab layout methods
    void createLayout();
    void connectLayout();

    //STR
    QPushButton *STRokay;
    QWidget *STRwindow;
    QLabel *title;
    QLabel *low;
    QLabel *high;
    QLabel *replacement_lab;
    QLineEdit *low_value;
    QLineEdit *high_value;
    QLineEdit *replacement_value;
    QLineEdit *radius_value;
    QGridLayout *options_layout;
    QComboBox *contour_list;
    int indexMARContour = -1;

};






#endif
