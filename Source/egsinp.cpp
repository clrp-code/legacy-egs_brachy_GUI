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

/*
egsinp.cpp

This class is used to create the seed transformation and egsinp files
It also is used to get the location of the spectra and muen file (if it hasnt been specified/selected by the user)

*/

//#include <chrono>
//#include <boost/algorithm/string.hpp>

#include "egsinp.h"




using namespace std;



/***
Function: create_egsinp
-----------------------
Process: Outputs the egsinp file

Inputs: element: The name of the egs_brachy folder containing the seed model (a folder in egs_brachy/lib/geometry/sources/)
        seed_found: The seed model, corresponding to the name in egs_brachy element
        DSF: the calculated dose scaling factor (calculated in get_from_RT)
        treatment_type: Brachy treatment type (MANUAL, HDR, MDR, LDR, PDR)
        source_weights: QVector containing the weighted dwell times
        unique_tissue: QVector of tissues in the egsphant (corresponding to the tissue assignement scheme)
        tg43Flag: the type of tissue assignment scheme (breast, prostate...)
        checked_score_eneg_deposition: flag that indicates if the socre energy deposisiton is checked
        user_muen_file: file name if user has uploaded own muen file
        number_hist: double that contains the number of history value if the user has decided to change it
        header: A user-specified string to be used as the header for the egsinp file
        transport_param_path: the transport parameter path selected by the user
        user_material_file: the material.dat file selected by the user
        zipped: value is true of the egsphant has been compressed using gzip
***/
void egsinp::create_egsinp(QString element, QString seed_found, double DSF, QString treatment_type,
                           QVector <double> source_weights, QVector<QString> unique_tissue, bool tg43Flag,
                           bool checked_score_eneg_deposition, QString user_muen_file, double number_hist, double number_batch,
                           double number_chunk, QString header, QString transport_param_path, QString user_material_file) {


    // Output the egs input file
    QFile *file;
    QTextStream *egsinp_file;
    file = new QFile(egsinp_path);


    auto time = std::chrono::system_clock::now();
    time_t date_time = std::chrono::system_clock::to_time_t(time);

    double nbatch = 1;
    double nchunk = 10;
    double ncase = 1e8;
    double AE = 0.512;
    double UE = 2.012;
    double AP = 0.001;
    double UP = 1.500;

    QString phantom_geometry = "phantom";
    QString EGS_ASwitchedEnvelope_geometry = "final";
    QString simulation_geometry = "final";
    QString score_energy_deposition = "no";
    QString transport_file;
    QString material_file;
    QString run_mode;


    get_spectra_file_path(element);

    if (user_material_file.isEmpty()) {
        material_file = egs_home + material_data;
    }
    else {
        material_file = user_material_file;
    }



    if ((treatment_type == "HDR") || (tg43Flag)) {
        run_mode = "superposition";    //no interseed effects
    }
    else {
        run_mode = "normal";
    }

    if ((treatment_type == "HDR")) {
        transport_file = egs_home + high_energy_transport_file;    //no interseed effects
    }
    else {
        transport_file= egs_home + low_energy_transport_file;
    }

    //Getting the transport parameter
    if (!transport_param_path.isEmpty()) {
        transport_file = transport_param_path;
    }


    //Changing default values based on the advanced options user pane
    if (checked_score_eneg_deposition) {
        score_energy_deposition = "yes";
    }

    if (number_hist != 0) {
        ncase = number_hist;
    }
    if (number_batch != 0) {
        nbatch = number_batch;
    }
    if (number_chunk != 0) {
        nchunk = number_chunk;
    }

    QString muen_file;
    if (!user_muen_file.isEmpty()) { //User has selected the file
        muen_file = egs_home + "/lib/muen/" + user_muen_file;
    }
    else {
        muen_file = egs_home + muen_file_substring;
    }



//----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

    if (file->open(QIODevice::WriteOnly | QIODevice::Text)) {
        egsinp_file = new QTextStream(file);

        // Header Data
        *egsinp_file<<"####################################################################################################" <<endl;
        *egsinp_file<<"#####" <<endl;
        *egsinp_file<<"#####  This is an automatically generated Patient Input File created on: " <<ctime(&date_time) <<endl;
        //if(!header.isEmpty())
        *egsinp_file <<"####" <<header <<endl;
        *egsinp_file<<"#####" <<endl;
        *egsinp_file<<"####################################################################################################" <<endl;

        // Body

        /***RUN CONTROL BLOCK
                histories, nbatch, nchunk, egsdat file format, geometry, error limit ***/
        *egsinp_file<<":start run control:" <<endl;
        *egsinp_file<<"	ncase = " <<ncase <<endl;   //variable
        *egsinp_file<<"	nbatch = " <<nbatch <<endl; //checkpoints where the results are written to the data file
        *egsinp_file<<"	nchunk = " <<nchunk <<endl; //value indicates no parallel processing ('chunks' cna be allocated to differnet machines -save time
        *egsinp_file<<"	calculation = first" <<endl;
        *egsinp_file<<"	geometry error limit = 2500" <<endl;
        *egsinp_file<<"	egsdat file format = gzip" <<endl;
        *egsinp_file<<":stop run control:" <<endl <<endl;
        *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;

        /***RUN MODE BLOCK ***/
        *egsinp_file<<":start run mode:" <<endl;
        *egsinp_file<<"	run mode = " <<run_mode <<endl; //variable: normal, superposition, volume correction
        *egsinp_file<<":stop run mode:" <<endl <<endl;
        *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;

        /***MEDIA DEFINITION
                -required in pegless mode (photon cross sections ***/
        *egsinp_file<<":start media definition:" <<endl;
        *egsinp_file<<"    AE = " <<AE <<endl;  //variables: lower energy threshold for creation of charged particles
        *egsinp_file<<"    UE = " <<UE <<endl;  //variables: upper energy threshold for creation of charged particles
        *egsinp_file<<"    AP = " <<AP <<endl;  //variables: lower energy threshold for creation of photons (recommended value)
        *egsinp_file<<"    UP = " <<UP <<endl;  //variables: upper energy threshold for creation of photons

        *egsinp_file<<"    material data file = " <<material_file <<endl <<endl;    //variable -local copy of material file
        *egsinp_file<<":stop media definition:" <<endl <<endl;
        *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;


        *egsinp_file<<":start geometry definition:" <<endl;
        *egsinp_file<<"    source geometries = " <<seed_found <<endl;   //which geometries define the actual brachytherapy source object
        *egsinp_file<<"    phantom geometries = " <<phantom_geometry <<endl;    //which geometries to score dose in
        if (run_mode == "superposition") {
            *egsinp_file<<"    source envelope geometry =" <<EGS_ASwitchedEnvelope_geometry <<endl;
        }
        *egsinp_file<<"    simulation geometry = " <<simulation_geometry <<endl <<endl;

        //Each start geometry is used to identify individual components of the geometry
        //geometry names must be unique - not checked in egs++
        if (!boxFilePath.isEmpty()) {
            *egsinp_file<<"	:start geometry:" <<endl;
            *egsinp_file<<"		name = box " <<endl;
            *egsinp_file<<"		library = egs_glib" <<endl;
            *egsinp_file<<"		include file = " <<boxFilePath <<endl;
            *egsinp_file<<"	:stop geometry:" <<endl <<endl;
        }

        *egsinp_file<<"	:start geometry:" <<endl;
        *egsinp_file<<"	    name = phantom" <<endl;
        *egsinp_file<<"	    library = egs_glib" <<endl;
        *egsinp_file<<"	    type = egsphant" <<endl;
        *egsinp_file<<"	    egsphant file = " <<egsphant_location <<".gz" <<endl;
        *egsinp_file<<"	    density file = " <<material_file <<endl;
        *egsinp_file<<"	:stop geometry:" <<endl <<endl;

        if (!boxFilePath.isEmpty()) {
            *egsinp_file<<"	# embed the scoring phantom in the box" <<endl;
            *egsinp_file<<"	:start geometry:" <<endl;
            *egsinp_file<<"		name = box_with_phantom" <<endl;
            *egsinp_file<<"		library = egs_genvelope" <<endl;
            *egsinp_file<<"		base geometry = box" <<endl;
            *egsinp_file<<"		inscribed geometries = phantom" <<endl;
            *egsinp_file<<"	:stop geometry:" <<endl <<endl;
        }

        *egsinp_file<<"	:start geometry:" <<endl;
        *egsinp_file<<"		name = " <<seed_found <<endl;
        *egsinp_file<<"		library = egs_glib" <<endl;
        *egsinp_file<<"		include file = " <<egs_home <<"/lib/geometry/sources/" <<element <<"/" <<seed_found <<"/" <<seed_found <<".geom" <<endl;
        *egsinp_file<<"	:stop geometry:" <<endl <<endl;

        if (!applicatorFilePath.isEmpty()) {
            *egsinp_file<<"	:start geometry:" <<endl;
            *egsinp_file<<"		name = applicator " <<endl;
            *egsinp_file<<"		library = egs_glib" <<endl;
            *egsinp_file<<"		include file = " <<applicatorFilePath <<endl;
            *egsinp_file<<"	:stop geometry:" <<endl <<endl;

            *egsinp_file<<"	:start geometry:" <<endl;   //put source in geometry
            *egsinp_file<<"		name = applicator_w_seeds" <<endl;
            *egsinp_file<<"		library = egs_autoenvelope" <<endl;
            *egsinp_file<<"		base geometry = applicator" <<endl <<endl;
            *egsinp_file<<"		:start inscribed geometry:" <<endl;
            *egsinp_file<<"			inscribed geometry name = " <<seed_found <<endl;
            *egsinp_file<<"			:start transformations:" <<endl;
            *egsinp_file<<"				include file = " <<seed_file <<endl;
            *egsinp_file<<"			:stop transformations:" <<endl;
            *egsinp_file<<"			:start region discovery:" <<endl;
            *egsinp_file<<"				action = discover" <<endl;
            *egsinp_file<<"				density of random points (cm^-3) = 1E8" <<endl;
            *egsinp_file<<"				include file = " <<egs_home <<"/lib/geometry/sources/"  <<element <<"/" <<seed_found <<"/boundary.shape" <<endl;
            *egsinp_file<<"			:stop region discovery:" <<endl;
            *egsinp_file<<"		:stop inscribed geometry:" <<endl;
            *egsinp_file<<"	:stop geometry:" <<endl <<endl;


            *egsinp_file<<"	:start geometry:" <<endl;
            *egsinp_file<<"    	name = final" <<endl;
            *egsinp_file<<"    	library = egs_genvelope" <<endl;
            if (run_mode == "superposition") {
                *egsinp_file<<"		type = EGS_ASwitchedEnvelope" <<endl;
            }
            if (boxFilePath.isEmpty()) {
                *egsinp_file<<"		base geometry = phantom"  <<endl;
            }
            else {
                *egsinp_file<<"		base geometry = box_with_phantom" <<endl;
            }

            *egsinp_file<<"    	inscribed geometries = applicator_w_seeds" <<endl;
            *egsinp_file<<"	:stop geometry:" <<endl <<endl;

            *egsinp_file<<":stop geometry definition:" <<endl <<endl;
            *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;
        }
        else {

            *egsinp_file<<"	:start geometry:" <<endl;   //put source in geometry
            *egsinp_file<<"		name = final" <<endl;

            *egsinp_file<<"		library = egs_autoenvelope" <<endl;

            if (run_mode == "superposition") {
                *egsinp_file<<"		type = EGS_ASwitchedEnvelope" <<endl;
            }

            if (boxFilePath.isEmpty()) {
                *egsinp_file<<"		base geometry = phantom" <<endl <<endl;
            }
            else {
                *egsinp_file<<"		base geometry = box_with_phantom" <<endl <<endl;
            }

            *egsinp_file<<"		:start inscribed geometry:" <<endl;
            *egsinp_file<<"			inscribed geometry name = " <<seed_found <<endl;
            *egsinp_file<<"			:start transformations:" <<endl;
            *egsinp_file<<"				include file = " <<seed_file <<endl;
            *egsinp_file<<"			:stop transformations:" <<endl;
            *egsinp_file<<"			:start region discovery:" <<endl;
            *egsinp_file<<"				action = discover" <<endl;
            *egsinp_file<<"				density of random points (cm^-3) = 1E8" <<endl;
            *egsinp_file<<"				include file = " <<egs_home <<"/lib/geometry/sources/"  <<element <<"/" <<seed_found <<"/boundary.shape" <<endl;
            *egsinp_file<<"			:stop region discovery:" <<endl;
            *egsinp_file<<"		:stop inscribed geometry:" <<endl;
            *egsinp_file<<"	:stop geometry:" <<endl <<endl;
            *egsinp_file<<":stop geometry definition:" <<endl <<endl;
            *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;

        }

        /***VOLUME CORRECTION ***/
        *egsinp_file<<":start volume correction:" <<endl <<endl;
        *egsinp_file<<"    :start source volume correction:" <<endl;
        if (treatment_type == "HDR") {
            *egsinp_file<<"        correction type = none" <<endl;    //values: correct, none, zero dose
        }
        else {
            *egsinp_file<<"        correction type = correct" <<endl;    //values: correct, none, zero dose
        }

        //*egsinp_file<<"        correction type = none" <<endl;    //values: correct, none, zero dose
        *egsinp_file<<"        density of random points (cm^-3) = 1E8" <<endl;  //usually between 1E6, 1E8
        *egsinp_file<<"        include file = " <<egs_home <<"/lib/geometry/sources/"  <<element <<"/" <<seed_found <<"/boundary.shape" <<endl;
        *egsinp_file<<"    :stop source volume correction:" <<endl <<endl;
        *egsinp_file<<":stop volume correction:" <<endl <<endl;
        *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;

        /*** SOURCE DEFINITION radioactivity in the seeds/sources ***/
        *egsinp_file<<":start source definition:" <<endl <<endl;
        *egsinp_file<<"    :start source:" <<endl;
        *egsinp_file<<"        name = " <<seed_found <<endl;    //seed name, not isotope name
        *egsinp_file<<"        library = egs_isotropic_source" <<endl;
        *egsinp_file<<"        charge = 0" <<endl;  //value is 0,+/-1
        *egsinp_file<<"        include file = " <<egs_home <<"/lib/geometry/sources/"  <<element <<"/" <<seed_found <<"/" <<seed_found <<".shape" <<endl <<endl;
        *egsinp_file<<"        :start spectrum:" <<endl;
        *egsinp_file<<"            type = tabulated spectrum" <<endl;
        *egsinp_file<<"            spectrum file = " <<spectra_file <<endl;
        *egsinp_file<<"        :stop spectrum:" <<endl <<endl;
        *egsinp_file<<"    :stop source:" <<endl <<endl;
        *egsinp_file<<"    :start transformations:" <<endl;
        *egsinp_file<<"        include file = " <<seed_file <<endl; //the location of transformatino file
        *egsinp_file<<"    :stop transformations:" <<endl <<endl;

        egsinp_file->setRealNumberPrecision(3);
        if (!source_weights.empty()) {
            *egsinp_file<<"    source weights = " <<source_weights[0];
            for (int i=1; i< source_weights.size(); i++) {
                *egsinp_file<<"," <<source_weights[i];    //QString::number(, 'f', 3);
            }

            *egsinp_file<<endl;
        }

        *egsinp_file<<"    simulation source = " <<seed_found <<endl;
        *egsinp_file<<":stop source definition:" <<endl <<endl;
        *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;

        /***SCORING OPTIONS***/
        *egsinp_file<<":start scoring options:" <<endl;
        if (tg43Flag) { //only medium is water
            *egsinp_file<<"    muen file = " <<muen_file <<endl;
            *egsinp_file<<"    muen for media = WATER_0.998" <<endl;
        }
        else {
            *egsinp_file<<"    score energy deposition = " <<score_energy_deposition <<endl;
            *egsinp_file<<"    muen file = " <<muen_file <<endl;
            *egsinp_file<<"    muen for media = ";//medium you wish to score dose data for in muen file, media names in egsphant
            for (int i=0; i < unique_tissue.size(); i++) {
                *egsinp_file << unique_tissue[i];
                if (i != unique_tissue.size()-1) {
                    *egsinp_file <<", ";
                }
            }
            *egsinp_file<<endl;
        }
        egsinp_file->setRealNumberNotation(QTextStream::SmartNotation);
        egsinp_file->setRealNumberPrecision(18);
        if (DSF != 0) {
            *egsinp_file<<"    dose scaling factor = " <<DSF <<endl;
        }
        *egsinp_file<<":stop scoring options:" <<endl <<endl;

        *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;

        /***MC trasport parameters***/
        *egsinp_file<<"include file = " <<transport_file <<endl;
        file->close();
        std::cout<<"Successfully created egsinp. Location: " <<egsinp_path.toStdString() <<endl;

    }
    else {
        std::cout << egsinp_path.toStdString() << " could not be created." << endl;
        //bool created_egsinput_file = false;
    }


}



/***
Function: create_transformation_file
------------------------------------
Process: creates the trasnformation file of the seed locations

Inputs: seed_data: the parsed seed data from the dicom file containing the dwell positions
        air_kerma: the air kerma value obtained from the DICOM files

***/
void egsinp::create_transformation_file(QVector<QVector<coordinate>> all_seed_pos, double air_kerma) {
    //Output the transformation to a file
    //-----------------------------------
    QFile file(seed_file);

    // Open up the file specified at path
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream output(&file);

        output<<"# Seed Location Tranformations" <<endl;
        output<<"# Air Kerma Strength = " <<air_kerma <<endl;

        for (int i=0; i<all_seed_pos.size(); i++) {
            for (int j=0; j<all_seed_pos[i].size(); j++) {
                output<<"# Seed " <<i+1 <<endl;
                output<<":start transformation:" <<endl;
                output<<"    translation = " <<all_seed_pos[i][j].x <<" " <<all_seed_pos[i][j].y <<" " <<all_seed_pos[i][j].z <<endl;
                //output<<"    rotation = " <<"0 0 0" <<endl; //get rotation data
                output<<":stop transformation:" <<endl <<endl;
            }
        }


        file.close();
        cout<<"Successfully created the source transformation file" <<seed_file.toStdString() <<"\n";
    }
    else {
        cout<<"ERROR: couldn't write to the source transformation file" <<seed_file.toStdString() <<"\n";
    }

}







/***
Function: get_sepctra_file_path
-------------------------------
Process: Reads the list of spectrum files form the default_files.txt file and parses them
         to try to match the appropriate file

***/
void egsinp::get_spectra_file_path(QString element) {
    int index = element.indexOf("_");
    element = element.chopped(index);

    //this loop is used to try ot identify the line in the text file that corresponds to the
    //element name, then extracts the name of spectrum file
    int spectra_size = spectra_files.size();
    for (int i=0; i<spectra_size; i++) {
        if (spectra_files[i].contains(element, Qt::CaseInsensitive)) {
            spectra_file = egs_home + spectra_files[i];
            read_spectra_file = true;
            //std::cout << "Successfully obtained spectra file name corresponding to the source." << endl;
        }
    }


    if (!read_spectra_file) { //unable to find/read the spectra file, ask user to select it
        QString fileName = QFileDialog::getOpenFileName(
                               0, tr("Select the spectra File to open"),
                               egs_home, "spectrum (*.spectrum)");


        if (!fileName.isEmpty()) {
            spectra_file = fileName;
        }
        else {
            std::cout<<"Warning: Unable to obtain the spectra file" <<endl;
        }

    }



}





/***
Function: create_egsinp_phantom
-----------------------
Process: Outputs the egsinp file

Inputs: element: The name of the egs_brachy folder containing the seed model (a folder in egs_brachy/lib/geometry/sources/)
        seed_found: The seed model, corresponding to the name in egs_brachy element
        seed_file: path of the seed transformation file
        phant_file: phant of the phantom
        run_mode: value of the run mode as selected by user
        checked_energy_depos: flag that indicates if the socre energy deposisiton is checked
        selected_muen_file: file path of muen file if user has selected one, empty otherwise
        numb_histories: The number of histories value if the user has decided to change it
        egsinp_header: A user-specified string to be used as the header for the egsinp file
***/

//QVector<QString> unique_tissue
//QString tg43Flag

void egsinp::create_egsinp_phantom(QString element, QString seed_found, QString phant_file,
                                   QString transp_file, QString run_mode, bool checked_energy_depos,
                                   QString selected_muen_file, double numb_histories, double numb_batch,
                                   double numb_chunk, QString egsinp_header, QString material_data_file) {


    // Output the egs input file
    QFile *filep;
    QTextStream *egsinp_file;
    filep = new QFile(egsinp_path);


    auto time = std::chrono::system_clock::now();
    time_t date_time = std::chrono::system_clock::to_time_t(time);

    double ncase = 1e8;
    double nbatch = 1;
    double nchunk = 10;
    double AE = 0.512;
    double UE = 2.012;
    double AP = 0.001;
    double UP = 1.500;
    bool gzip = false;

    QString phantom_geometry = "phantom";
    QString EGS_ASwitchedEnvelope_geometry = "final";
    QString simulation_geometry = "final";
    QString score_energy_deposition = "no";
    QString material_file;

    get_spectra_file_path(element);

    if (material_data_file.isEmpty()) {
        material_file = egs_home + material_data;
    }
    else {
        material_file = material_data_file;
    }



    //Changing default values based on the advanced options user pane
    if (checked_energy_depos) {
        score_energy_deposition = "yes";
    }

    if (numb_histories != 0) {
        ncase = numb_histories;
    }
    if (numb_batch != 0) {
        nbatch = numb_batch;
    }
    if (numb_chunk != 0) {
        nchunk = numb_chunk;
    }

    QString muen_file;
    if (!selected_muen_file.isEmpty()) {
        std::cout<<"muen not selected  ";
        muen_file = egs_home + "/lib/muen/" + selected_muen_file;
        std::cout<<muen_file.toStdString() <<"\n";
    }
    else {
        muen_file =egs_home + muen_file_substring;
        std::cout<<"muen chosen by user " <<muen_file.toStdString() <<"\n";
    }

//  //Getting the media from the phantom
//          //Searches for "media =" and creates a vector caontaining each media
    QString search_for = "media = ";

    //need to unzip file
    if (phant_file.endsWith(".gz")) {
        gzip = true;
        std::cout<<"Need to unzip the egsphant file to obtain the media \n";
        QProcess *qzip = new QProcess();
        qzip->setProcessChannelMode(QProcess::MergedChannels);
        qzip->setReadChannel(QProcess::StandardOutput);

        //Actual line of exectuion
        QString command = "gunzip " + phant_file;
        qzip->execute(command);
        phant_file.remove(phant_file.size()-3, 3);
    }



    QStringList mediaList;
    QFile *file = new QFile(phant_file);
    if (file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream input(file);
        if (phant_file.endsWith(".egsphant")) {
            //egsphant format
            int num_media =  input.readLine().toInt(); //first line is num media
            int i=0;
            while (i < num_media) { //next lines are the media
                mediaList << input.readLine().trimmed();
                i++;
            }
        }
        else {   //geom format, look for 'media ='

            while (!input.atEnd()) {
                QString line = input.readLine();
                if (line.contains("media =")) {
                    line.remove("media =");
                    mediaList = line.split(',');
                }

            }
        }
    }
    else {
        std::cout << "Was not able to read media from " << phant_file.toStdString() << "\n";
    }
    delete file;

//----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

    if (filep->open(QIODevice::WriteOnly | QIODevice::Text)) {
        egsinp_file = new QTextStream(filep);

        // Header Data
        *egsinp_file<<"####################################################################################################" <<endl;
        *egsinp_file<<"#####" <<endl;
        *egsinp_file<<"#####  This is an automatically generated Patient Input File created on: " <<ctime(&date_time) <<endl;
        if (!egsinp_header.isEmpty()) {
            *egsinp_file <<"####" <<egsinp_header <<endl;
        }
        *egsinp_file<<"#####" <<endl;
        *egsinp_file<<"####################################################################################################" <<endl;

        // Body

        /***RUN CONTROL BLOCK
                histories, nbatch, nchunk, egsdat file format, geometry, error limit ***/
        *egsinp_file<<":start run control:" <<endl;
        *egsinp_file<<"	ncase = " <<ncase <<endl;   //variable
        *egsinp_file<<"	nbatch = " <<nbatch <<endl; //checkpoints where the results are written to the data file
        *egsinp_file<<"	nchunk = " <<nchunk <<endl; //value indicates no parallel processing ('chunks' cna be allocated to differnet machines -save time
        *egsinp_file<<"	calculation = first" <<endl;
        *egsinp_file<<"	geometry error limit = 1000" <<endl;
        if (gzip) {
            QProcess *qzip = new QProcess();
            qzip->setProcessChannelMode(QProcess::MergedChannels);
            qzip->setReadChannel(QProcess::StandardOutput);
            QString command = "gzip -f " + phant_file;  //-f used if exisitng .gz file, -f forces it
            qzip->execute(command);
            *egsinp_file<<"	egsdat file format = gzip" <<endl <<endl;
            phant_file.append(".gz");
        }
        *egsinp_file<<":stop run control:" <<endl <<endl;
        *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;

        /***RUN MODE BLOCK ***/
        *egsinp_file<<":start run mode:" <<endl;
        *egsinp_file<<"	run mode = " <<run_mode <<endl; //variable: normal, superposition, volume correction
        *egsinp_file<<":stop run mode:" <<endl <<endl;
        *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;

        /***MEDIA DEFINITION
                -required in pegless mode (photon cross sections ***/
        *egsinp_file<<":start media definition:" <<endl;
        *egsinp_file<<"    AE = " <<AE <<endl;  //variables: lower energy threshold for creation of charged particles
        *egsinp_file<<"    UE = " <<UE <<endl;  //variables: upper energy threshold for creation of charged particles
        *egsinp_file<<"    AP = " <<AP <<endl;  //variables: lower energy threshold for creation of photons (recommended value)
        *egsinp_file<<"    UP = " <<UP <<endl;  //variables: upper energy threshold for creation of photons

        *egsinp_file<<"    material data file = " <<material_file <<endl <<endl;    //variable -local copy of material file
        *egsinp_file<<":stop media definition:" <<endl <<endl;
        *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;

        /***Geometry Definition ***/
        *egsinp_file<<":start geometry definition:" <<endl;
        *egsinp_file<<"    source geometries = " <<seed_found <<endl;   //which geometries define the actual brachytherapy source object
        *egsinp_file<<"    phantom geometries = " <<phantom_geometry <<endl;    //which geometries to score dose in
        if (run_mode == "superposition") {
            *egsinp_file<<"    source envelope geometry =" <<EGS_ASwitchedEnvelope_geometry <<endl;
        }
        *egsinp_file<<"    simulation geometry = " <<simulation_geometry <<endl <<endl;

        //Each start geometry is used to identify individual components of the geometry
        //geometry names must be unique - not checked in egs++
        if (!boxFilePath.isEmpty()) {
            *egsinp_file<<"	:start geometry:" <<endl;
            *egsinp_file<<"		name = box " <<endl;
            *egsinp_file<<"		library = egs_glib" <<endl;
            *egsinp_file<<"		include file = " <<boxFilePath <<endl;
            *egsinp_file<<"	:stop geometry:" <<endl <<endl;
        }

        *egsinp_file<<"    :start geometry:" <<endl;
        *egsinp_file<<"        name = phantom" <<endl;
        *egsinp_file<<"        library = egs_glib" <<endl;
        if (phant_file.endsWith(".egsphant.gz") || phant_file.endsWith(".egsphant")) {
            *egsinp_file<<"        type = egsphant" <<endl;
            *egsinp_file<<"        egsphant file = " <<phant_file  <<endl;
            *egsinp_file<<"        density file = " <<material_file  <<endl;
        }
        else {
            *egsinp_file<<"        include file = " <<phant_file <<endl;
        }
        *egsinp_file<<"    :stop geometry:" <<endl <<endl;

        if (!boxFilePath.isEmpty()) {
            *egsinp_file<<"	# embed the scoring phantom in the box" <<endl;
            *egsinp_file<<"	:start geometry:" <<endl;
            *egsinp_file<<"		name = box_with_phantom" <<endl;
            *egsinp_file<<"		library = egs_genvelope" <<endl;
            *egsinp_file<<"		base geometry = box" <<endl;
            *egsinp_file<<"		inscribed geometries = phantom" <<endl;
            *egsinp_file<<"	:stop geometry:" <<endl <<endl;
        }

        *egsinp_file<<"    :start geometry:" <<endl;
        *egsinp_file<<"		name = " <<seed_found <<endl;
        *egsinp_file<<"		library = egs_glib" <<endl;
        *egsinp_file<<"		include file = " <<egs_home <<"/lib/geometry/sources/" <<element <<"/" <<seed_found <<"/" <<seed_found <<".geom" <<endl;
        *egsinp_file<<"    :stop geometry:" <<endl <<endl;


        if (!applicatorFilePath.isEmpty()) {
            *egsinp_file<<"	:start geometry:" <<endl;
            *egsinp_file<<"		name = applicator " <<endl;
            *egsinp_file<<"		library = egs_glib" <<endl;
            *egsinp_file<<"		include file = " <<applicatorFilePath <<endl;
            *egsinp_file<<"	:stop geometry:" <<endl <<endl;

            // *egsinp_file<<"  :start geometry:" <<endl;   //put source in geometry
            // *egsinp_file<<"      name = applicator_w_seeds" <<endl;
            // *egsinp_file<<"      library = egs_autoenvelope" <<endl;
            // *egsinp_file<<"      base geometry = applicator" <<endl <<endl;
            // *egsinp_file<<"      :start inscribed geometry:" <<endl;
            // *egsinp_file<<"          inscribed geometry name = " <<seed_found <<endl;
            // *egsinp_file<<"          :start transformations:" <<endl;
            // *egsinp_file<<"              include file = " <<seed_file <<endl;
            // *egsinp_file<<"          :stop transformations:" <<endl;
            // *egsinp_file<<"          :start region discovery:" <<endl;
            // *egsinp_file<<"              action = discover" <<endl;
            // *egsinp_file<<"              density of random points (cm^-3) = 1E8" <<endl;
            // *egsinp_file<<"              include file = " <<egs_home <<"/lib/geometry/sources/"  <<element <<"/" <<seed_found <<"/boundary.shape" <<endl;
            // *egsinp_file<<"          :stop region discovery:" <<endl;
            // *egsinp_file<<"      :stop inscribed geometry:" <<endl;
            // *egsinp_file<<"  :stop geometry:" <<endl <<endl;

            *egsinp_file<<"	:start geometry:" <<endl;   //put source in geometry
            *egsinp_file<<"		name = applicator_w_seeds" <<endl;
            *egsinp_file<<"		library = egs_glib" <<endl;
            *egsinp_file<<"		base geometry = applicator" <<endl <<endl;
            *egsinp_file<<"			inscribed geometry name = " <<seed_found <<endl;
            *egsinp_file<<"	:stop geometry:" <<endl <<endl;


            *egsinp_file<<"	:start geometry:" <<endl;
            *egsinp_file<<"    	name = final" <<endl;
            *egsinp_file<<"    	library = egs_genvelope" <<endl;
            if (run_mode == "superposition") {
                *egsinp_file<<"		type = EGS_ASwitchedEnvelope" <<endl;
            }
            if (boxFilePath.isEmpty()) {
                *egsinp_file<<"		base geometry = phantom"  <<endl;
            }
            else {
                *egsinp_file<<"		base geometry = box_with_phantom" <<endl;
            }

            *egsinp_file<<"    	inscribed geometries = applicator_w_seeds" <<endl;
            *egsinp_file<<"	:stop geometry:" <<endl <<endl;

            *egsinp_file<<":stop geometry definition:" <<endl <<endl;
            *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;
        }
        else {

            *egsinp_file<<"	:start geometry:" <<endl;   //put source in geometry
            *egsinp_file<<"		name = final" <<endl;
            if (boxFilePath.isEmpty()) {
                *egsinp_file<<"		library = egs_autoenvelope" <<endl;
            }
            else {
                *egsinp_file<<"		library = egs_genvelope" <<endl;
            }

            if (run_mode == "superposition") {
                *egsinp_file<<"		type = EGS_ASwitchedEnvelope" <<endl;
            }

            if (boxFilePath.isEmpty()) {
                *egsinp_file<<"		base geometry = phantom" <<endl <<endl;
                *egsinp_file<<"		:start inscribed geometry:" <<endl;
                *egsinp_file<<"			inscribed geometry name = " <<seed_found <<endl;
                *egsinp_file<<"			:start transformations:" <<endl;
                *egsinp_file<<"				include file = " <<seed_file <<endl;
                *egsinp_file<<"			:stop transformations:" <<endl;
                *egsinp_file<<"			:start region discovery:" <<endl;
                *egsinp_file<<"				action = discover" <<endl;
                *egsinp_file<<"				density of random points (cm^-3) = 1E8" <<endl;
                *egsinp_file<<"				include file = " <<egs_home <<"/lib/geometry/sources/"  <<element <<"/" <<seed_found <<"/boundary.shape" <<endl;
                *egsinp_file<<"			:stop region discovery:" <<endl;
                *egsinp_file<<"		:stop inscribed geometry:" <<endl;

            }
            else {
                *egsinp_file<<"		base geometry = box_with_phantom" <<endl <<endl;
                *egsinp_file<<"		inscribed geometry name = " <<seed_found <<endl;
            }

            *egsinp_file<<"	:stop geometry:" <<endl <<endl;
            *egsinp_file<<":stop geometry definition:" <<endl <<endl;
            *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;
        }

        /***VOLUME CORRECTION ***/
        *egsinp_file<<":start volume correction:" <<endl <<endl;
        *egsinp_file<<"    :start source volume correction:" <<endl;
        *egsinp_file<<"        correction type = correct" <<endl;   //values: correct, none, zero dose
        *egsinp_file<<"        density of random points (cm^-3) = 1E8" <<endl;  //usually between 1E6, 1E8
        *egsinp_file<<"        include file = " <<egs_home <<"/lib/geometry/sources/"  <<element <<"/" <<seed_found <<"/boundary.shape" <<endl;
        *egsinp_file<<"    :stop source volume correction:" <<endl <<endl;
        *egsinp_file<<":stop volume correction:" <<endl <<endl;
        *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;

        /*** SOURCE DEFINITION radioactivity in the seeds/sources ***/
        *egsinp_file<<":start source definition:" <<endl <<endl;
        *egsinp_file<<"    :start source:" <<endl;
        *egsinp_file<<"        name = " <<seed_found <<endl;    //seed name, not isotope name
        *egsinp_file<<"        library = egs_isotropic_source" <<endl;
        *egsinp_file<<"        charge = 0" <<endl;  //value is 0,+/-1
        *egsinp_file<<"        include file = " <<egs_home <<"/lib/geometry/sources/"  <<element <<"/" <<seed_found <<"/" <<seed_found <<".shape" <<endl <<endl;
        *egsinp_file<<"        :start spectrum:" <<endl;
        *egsinp_file<<"            type = tabulated spectrum" <<endl;
        *egsinp_file<<"            spectrum file = " << spectra_file <<endl;
        *egsinp_file<<"        :stop spectrum:" <<endl <<endl;
        *egsinp_file<<"    :stop source:" <<endl <<endl;
        *egsinp_file<<"    :start transformations:" <<endl;
        *egsinp_file<<"        include file = " <<seed_file <<endl; //the location of transformatino file
        *egsinp_file<<"    :stop transformations:" <<endl <<endl;



        *egsinp_file<<"    simulation source = " <<seed_found <<endl;
        *egsinp_file<<":stop source definition:" <<endl <<endl;
        *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;

        /***SCORING OPTIONS***/
        *egsinp_file<<":start scoring options:" <<endl;
        *egsinp_file<<"    muen file = " <<muen_file <<endl;
        if (mediaList.size() >0) {
            *egsinp_file<<"    muen for media = ";  //get the media from the geom file
            *egsinp_file << mediaList[0].trimmed();
            for (int i=1; i < mediaList.size(); i++) {
                *egsinp_file <<", " <<mediaList[i].trimmed();
            }
        }
        *egsinp_file<<endl;
        *egsinp_file<<":stop scoring options:" <<endl <<endl;

        *egsinp_file<<"#----------------------------------------------------------------------------------------------------" <<endl <<endl;

        /***MC trasport parameters***/
        *egsinp_file<<"include file = " <<transp_file <<endl;

        filep->close();
        std::cout<<"Successfully created egsinp. Location: " <<egsinp_path.toStdString() <<endl;

    }
    else {
        std::cout << egsinp_path.toStdString() << " could not be created." << endl;
    }


}















