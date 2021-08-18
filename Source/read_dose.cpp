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

//Reads the .3ddose file and creates a DCIOM dose file

#include "read_dose.h"

//using namespace std;

/***
Function: load_dose_data
------------------------
Process: Used to read a 3ddose file

Inputs: path: the absolute path of the 3ddose file
***/
void read_dose::load_dose_data(QString path) {

    // Open the .3ddose file
    QFile *file;
    QTextStream *input;
    file = new QFile(path);

    // Set up the progress bar
    create_progress_bar();
    progress->reset();
    *remainder = 0;
    progWin->setWindowTitle("Importing 3ddose File");
    progWin->show();
    progWin->activateWindow();
    progWin->raise();


    // Determine the increment size of the status bar this 3ddose file gets
    double increment = 1000000000;

    //Opening and reading the 3ddose file
    if (file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        input = new QTextStream(file);

        updateProgress(increment*0.005);

        // Read in the number of voxels
        *input >> x;
        *input >> y;
        *input >> z;

        cx.resize(x+1);
        cy.resize(y+1);
        cz.resize(z+1);
        val.resize(x*y*z);

        updateProgress(increment*0.01);

        // Read in boundaries
        for (int i = 0; i <= x; i++) {
            *input >> cx[i];
        }
        for (int j = 0; j <= y; j++) {
            *input >> cy[j];
        }
        for (int k = 0; k <= z; k++) {
            *input >> cz[k];
        }

        updateProgress(increment*0.01);


        // Resize increments so that the rest of the section of the progress
        // bar is divided by two times the number of z values, so that each
        // time the loops iterate througha new z, the progress bar updates

        increment *= 0.975;
        increment = increment/(2*z);



        // Read in all the doses
        for (int k = 0; k < z; k++) {
            for (int j = 0; j < y; j++)
                for (int i = 0; i < x; i++) {
                    *input >> val[i+j*x+k*x*y];
                    if (i+j*x+k*x*y == 0) { //initialize the max, min dose vales
                        max_val = min_val = val[0];
                    }
                    if (val[i+j*x+k*x*y] > max_val) {
                        max_val = val[i+j*x+k*x*y];
                    }
                    if (val[i+j*x+k*x*y] < min_val) {
                        min_val = val[i+j*x+k*x*y];
                    }

                }
            updateProgress(increment);
        }

        if (!input->atEnd()) {
            // Read in all the errors
            err.resize(x*y*z);

            for (int k = 0; k < z; k++) {
                for (int j = 0; j < y; j++)
                    for (int i = 0; i < x; i++) {
                        *input >> err[i+j*x+k*x*y];
                    }

                updateProgress(increment);
            }
        }
        delete input;
    }

    delete file;


    progress->setValue(1000000000);
    progWin->hide();

    //Calculate some values
    x_thick = fabs(cx[0]-cx[1]);
    y_thick = fabs(cy[0]-cy[1]);
    z_thick = fabs(cz[0]-cz[1]);

    if (cz[0] > cz[1]) { //if z-values in decreasing order, flip them
        flip = true;
    }

}

/***
Function: AddRemainingTags
------------------------
Process: Used to add additional information/tags to the dicom tag storage

Inputs: path: the absolute path of the 3ddose file
***/
void read_dose::AddRemainingTags() {
    //-----------------------------------------------------------
    // Read all the default tags from the text file
    //-----------------------------------------------------------
    //dose_dict.dat contains all the tags to be output in the DICOM dose file
    //It inculdes pre-determined tag values and tags whose data is obtained
    //from the 3ddose/input DICOM files
    QString tempS;

    QFile *file = new QFile("dose_dict.dat");
    if (file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream input(file);
        element temp_element;
        while (!input.atEnd()) {
            input >> tempS;
            tempS = tempS.trimmed();

            if (tempS.left(3) == "tag") {
                temp_element.tag = tempS.mid(5,8);
            }
            else if (tempS.left(2) == "vr") {
                temp_element.VR = tempS.mid(4,2);
            }
            else if (tempS.left(5) == "value") {
                temp_element.value = tempS.left(tempS.length()-1).mid(7).replace(QString("_"), QString(" ")).replace(QString("-"), QString("_"));
                temp_element.length = temp_element.value.length();
            }
            else if (tempS.left(9) == "transform") {
                temp_element.transform = tempS.left(tempS.length()-1).mid(11);
                dictionary.append(temp_element);
                temp_element.tag.clear();
                temp_element.VR.clear();
                temp_element.value.clear();
                temp_element.transform.clear();

            }
        }
    }

    delete file;
    //-----------------------------------------------------------
    //Add additional required tags to the dicomHeader tag storage
    //------------------------------------------------------------
    QString root_uid = "1.2.826.0.1.3680043.10.240."; //add random numbers to the end of the uid root
    //int remaining_length = 64 - root_uid.length(); //length of the string must be less than 64 char

    time_t curr_time;
    tm *curr_tm;
    char date_string[16];

    time(&curr_time);
    curr_tm = localtime(&curr_time);
    strftime(date_string, 16, "%Y%m%d%H%M%S", curr_tm); //This is used as a unique number for my uid

    dicomHeader.insert("00080016", "RT Dose Storage"); //SOP Class UID

    int random_num = rand() % 100;
    QString SOPInstanceUID = root_uid + "1.4." + date_string + "." + QString::number(random_num);
    dicomHeader.insert("00080018", SOPInstanceUID);

    random_num = rand() % 100;
    QString StudyInstanceUID =root_uid + "1.2." + date_string + "." + QString::number(random_num);
    dicomHeader.insert("0020000d", StudyInstanceUID);

    random_num = rand() % 100;
    QString SeriesInstanceUID = root_uid + "1.3." + date_string + "." + QString::number(random_num);
    dicomHeader.insert("0020000e", SeriesInstanceUID);

    random_num = rand() % 100;
    QString FrameOfReferenceUID = root_uid +  "1.5." + date_string + "." + QString::number(random_num);
    dicomHeader.insert("00200052", FrameOfReferenceUID);

    char date[9];
    strftime(date, 9, "%Y%m%d", curr_tm);
    QString date_str(date);
    dicomHeader.insert("00080023", date_str);

    char time[7];
    strftime(time, 7, "%H%M%S", curr_tm);

    QString time_string(time);
    dicomHeader.insert("00080033", time_string);

}

/***

Function: create RT Dose file
----------------------------

Process: This function uses the 3ddose data to output a file in the RT Dose file format.
        Add the information form patient data of dicom input files to the dictionary



    //
    // Output Sequence: Tag (4 bytes, little endian), VR, length,  data
    //
***/
void read_dose::create_dicom_dose(QMap<QString,QString> dicomHeaderData, QString path) {
    if (!dicomHeaderData.isEmpty()) {
        dicomHeader = dicomHeaderData;
    }

    // Set up the progress bar
    create_progress_bar();
    progress->reset();
    *remainder = 0;
    progWin->setWindowTitle("Creating DICOM dose file");
    progWin->show();
    progWin->activateWindow();
    progWin->raise();

    unsigned char zero = 0x00;
    unsigned char space = 0x20;

    double increment = 1000000000;

    updateProgress(increment*0.05);

    updateProgress(increment*0.05);


    file_out.open(path.toStdString(), std::ios::out | std::ios::binary);

    if (!file_out.is_open()) {
        std::cout << "DICOM Dose file could not be created " <<path.toStdString() << endl;
        return;
    }

    //------------------------------------
    AddRemainingTags(); //add data to dicomHeader for additional tags (ie current date/time..)
    updateProgress(increment*0.05);

    get_3ddose_data();  //add data to dicomHeader from the 3ddose file
    updateProgress(increment*0.05);

    increment *= 0.985;
    increment /= (dictionary.size()*4);

    //output the preamble nad header
    file_out << std::string(128, zero) <<'D' <<'I' <<'C' <<'M';


    //std::cout<<"itterate through dictionary \n";
    int numOccurance = 0;
    int numOccurance2 = 0;
    //itterate through the tags in the dose dictionary, outputting the corresponding length and information
    for (int i=0; i<dictionary.size(); i++) {

        //Output the tag
        QVector<unsigned char> tag_vect = get_tag_output(dictionary[i].tag); //output the tag
        file_out<<tag_vect[0] <<tag_vect[1] <<tag_vect[2] <<tag_vect[3];

        updateProgress(increment);

        if (dictionary[i].transform != "delim") { //if delim, just want to output the tag

            //Output the VR
            std::string VR = dictionary[i].VR.toStdString(); //save the Vr to be accessed easier
            file_out<<VR[0] <<VR[1];    //output the VR
            bool odd_length = false;

            //Check if the value/length should be overridden with data from the dicomHeader
            if (dicomHeader.contains(dictionary[i].tag)) {
                if (dictionary[i].tag == "00081150") {
                    if (numOccurance == 0) { //Get the plan UID
                        dictionary[i].value = dicomHeader.value(dictionary[i].tag).split('\\',QString::SkipEmptyParts)[0];
                        numOccurance ++;
                    }
                    else {
                        dictionary[i].value = dicomHeader.value(dictionary[i].tag).split('\\',QString::SkipEmptyParts)[1];
                        numOccurance ++;
                    }
                }
                else if (dictionary[i].tag == "00081155") {
                    if (numOccurance == 0) { //Get the plan UID
                        dictionary[i].value = dicomHeader.value(dictionary[i].tag).split('\\',QString::SkipEmptyParts)[0];
                        numOccurance2 ++;
                    }
                    else {
                        dictionary[i].value = dicomHeader.value(dictionary[i].tag).split('\\',QString::SkipEmptyParts)[1];
                        numOccurance2 ++;
                    }
                }
                else {
                    dictionary[i].value = dicomHeader.value(dictionary[i].tag);
                }
                dictionary[i].length = dictionary[i].value.length();
            }
            //std::cout<<dictionary[i].tag.toStdString() <<" " <<dictionary[i].VR.toStdString() <<" " <<dictionary[i].length <<" " <<dictionary[i].value.toStdString() <<"\n";
            //output the length
            //-------------------------
            //Determines the length of the dicom value and adjusts if null padding is needed (odd length) or the data is for a 32-byte tag
            int length = dictionary[i].length;

            if (dictionary[i].transform == "tag") { //tag's have 2 characters corresp to one byte -length is half
                length = length/2;
            }

            if (VR == "US") {
                length =2;    //2 bytes fixed length for tags of this VR type
            }
            if (VR == "UL" || VR == "AT") {
                length =4;    //4 bytes fixed length for tags of this VR type
            }


            if (length %2 != 0) { //cant have odd length, adjust value for null padding
                length = length + 1;
                odd_length = true;
            }



            if (VR == "OB" || VR == "OW" || VR== "OF"  || VR == "UT" || VR == "UN") { //length is 32 (int get_from_RT SQ is 32-byte)
                if (dictionary[i].tag == "00020001") {
                    length = length/2;    //need to do this
                }

                if (dictionary[i].tag == "7fe00010") {
                    file_out<< zero << zero; //need to output zeros before the length
                    output_length_32_pixel(pixel_data.size()*4);
                }
                else {
                    output_length_32(length);
                }
            }
            else {
                output_length_16(length);
            }

            updateProgress(increment);
            //-------------------------


            //output the value
            //-------------------------
            if (dictionary[i].tag == "7fe00010") { //Pixel data tag

                output_pixel_data();

            }
            else if (dictionary[i].transform == "tag") {  //value is a tag or should be outputted like one

                QVector<unsigned char> tag_vect = get_tag_output(dictionary[i].value);
                file_out<<tag_vect[0] <<tag_vect[1] <<tag_vect[2] <<tag_vect[3];

            }
            else if (dictionary[i].transform == "number") {  //value is an int number, can output like the length

                if (dictionary[i].value.isEmpty()) { //If the number is empty, replace with zero
                    dictionary[i].value = "0";
                }

                int value = dictionary[i].value.toInt();
                output_length_16(value);


            }
            else {

                file_out<<dictionary[i].value.toStdString();

            }

            //need to fill end with a zero if odd data length
            if (odd_length) {
                if (VR == "LO" || VR == "SH" || VR == "TM" || VR == "UT" || VR == "ST" ||
                        VR == "PN" || VR == "LT" || VR == "IS" || VR == "DT" || VR == "AE" ||
                        VR == "CS" || VR == "DS") { //add trailing spaces
                    file_out<< space;
                }
                else {
                    file_out<<zero;
                }
            }

            updateProgress(increment);

        }

        updateProgress(increment);

    }

    std::cout<<"Successfully created the DICOM Dose file " <<path.toStdString() <<"\n";
    file_out.close();

    progress->setValue(1000000000);
    progWin->hide();

}


/***
Function: get_DoseGridScaling
-----------------------------
Process: Finds a mapping from Floating Point to Integer
         Finds the maximum number of decimals in the dose data
         The mapping factor is 10^(max number decimals)

         https://github.com/rordenlab/dcm2niix/issues/198
***/
void read_dose::get_DoseGridScaling() {

    scaling = 4290000000/max_val ;
    reverse_mapping_factor = 1/scaling;

}



/***
Function: get_3ddose_data
-------------------------
Process: Add the data from the dicom tags to the dose tag dictionary

00080023: today's date
***/
void read_dose::get_3ddose_data() {
    get_DoseGridScaling();

    dicomHeader.insert("00180050", QString::number(z_thick*10)); //Slice Thickness

    QString data_x = QString::number((cx[0] + x_thick/2)*10); //x-center in mm
    QString data_y =  QString::number((cy[0] + y_thick/2)*10); //y-center in mm
    QString data_z =  QString::number((cz[0] + z_thick/2)*10); //z-center in mm
    QString backslash = "\\";

    dicomHeader.insert("00200032", QString(data_x + backslash + data_y + backslash + data_z)); //Image position patient, xzy center of 1st voxel
    //output vector as num \ num \..

    dicomHeader.insert("00280008", QString::number(z)); //Number of z frames
    dicomHeader.insert("00280011",  QString::number(y)); //Number of columns
    dicomHeader.insert("00280010",  QString::number(x)); //Number of rows

    std::cout<<"x thick " <<x_thick <<" y thick " <<y_thick <<"\n";
    data_x =  QString::number(x_thick*10); //x-center in mm
    data_y =  QString::number(y_thick*10); //y-center in mm
    dicomHeader.insert("00280030",  QString::number(x_thick*10) + backslash +  QString::number(y_thick*10)); //Pixel Spacing


    QStringList grid_string;
    if (flip) { //start at the back
        double relative_zero = cz[cz.size()-1];

        for (int i=cz.size()-1; i>0; i--) {
            grid_string << QString::number((relative_zero - cz[i])*10);
        }
    }
    else {
        double relative_zero = cz[0];

        for (int i=0; i<cz.size()-1; i++) {
            grid_string << QString::number((cz[i] - relative_zero)*10);
        }
    }
    dicomHeader.insert("3004000c", grid_string.join("\\")); //Grid frame offset vector -vector of the z-planes
    //output in terms of relative coordinates

    //In traditional ct files the tags rescale intercept and rescale slope (0028 1052/1053) is used
    //In this case, we used the dose grid scaling
    dicomHeader.insert("3004000e", QString::number(reverse_mapping_factor)); //Dose Grid Scaling


    // tag == "7fe00010" //dose pixel data, convert float to int
    for (int j=0; j<val.size(); j++) {
        pixel_data.push_back(int(val[j]*scaling));
    }


}


/***
Function: output_pixel_data
---------------------------
Process: This function is used to output the pixel data

***/
void read_dose::output_pixel_data() {

    for (int i=0; i<pixel_data.size(); i++) {
        output_length_32_pixel(pixel_data[i]);
    }

}

/***
Function: output_length_32
--------------------------
Process: Used to output a length in a field of 4 bytes

***/
void read_dose::output_length_32(int length) {

    int32_t n = length;
    char data[4];
    data[2] = static_cast<char>(n & 0xFF);
    data[3] = static_cast<char>((n >> 8) & 0xFF);
    data[0] = static_cast<char>((n >> 16) & 0xFF);
    data[1] = static_cast<char>((n >> 24) & 0xFF);
    file_out.write(data, 4);

}


/***
Function: output_length_32_pixel
--------------------------------
Process: Used to output a length in a field of 4 bytes

***/
void read_dose::output_length_32_pixel(int length) {

    uint32_t n = length;
    char data[4];
    data[0] = static_cast<char>(n & 0xFF);
    data[1] = static_cast<char>((n >> 8) & 0xFF);
    data[2] = static_cast<char>((n >> 16) & 0xFF);
    data[3] = static_cast<char>((n >> 24) & 0xFF);
    file_out.write(data, 4);

}



/***
Function: output_length_16
--------------------------
Process: Used to output a length in a field of 2 bytes

Input: integer of length corresponding to the number of bytes in the dicom data
***/
void read_dose::output_length_16(int length) {
    int16_t n = length;
    char data[2];
    data[0] = static_cast<char>(n & 0xFF);
    data[1] = static_cast<char>((n >> 8) & 0xFF);
    file_out.write(data, 2);

}

/***
Functioon: get_tag_output
-------------------------
Process: Converts the DICOM tag to a vector of unsinged chars, in little endian format
***/
QVector<unsigned char> read_dose::get_tag_output(QString tag) {
    std::string tag1 = tag.toStdString();
    QVector<unsigned char> vector_tag;
    //extracting the two characters of the tag
    std::string c1 = tag1.substr(0,2);
    std::string c2 = tag1.substr(2,2);
    std::string c3 = tag1.substr(4,2);
    std::string c4 = tag1.substr(6,2);      //seqeunce c2c1 c4c3

    std::string seq1 = "0x" + c1; //converting to the correct format
    int hex1 = stoi(seq1, nullptr, 16);
    unsigned char value1 = hex1;

    std::string seq2 = "0x" + c2;
    int hex2 = stoi(seq2, nullptr, 16);
    unsigned char value2 = hex2;

    std::string seq3 = "0x" + c3;
    int hex3 = stoi(seq3, nullptr, 16);
    unsigned char value3 = hex3;

    std::string seq4 = "0x" + c4;
    int hex4 = stoi(seq4, nullptr, 16);
    unsigned char value4 = hex4;

    vector_tag.push_back(value2);
    vector_tag.push_back(value1);
    vector_tag.push_back(value4);
    vector_tag.push_back(value3); //adding the values in the propper order


    return vector_tag;

}


void read_dose::updateProgress(double n) {
    // The flooring function rounds down a real number to the nearest integer
    // In this line, we remove the remainder from the total number
    *remainder += n - floor(n);

    // Next we increment the progress bar by all of our whole numbers
    progress->setValue(int(progress->value() + floor(n) + floor(*remainder)));

    // We redraw the bar
    progress->update();
    QApplication::processEvents();

    // And if our remainder makes a whole number, remove it
    *remainder -= floor(*remainder);
}



void read_dose::create_progress_bar() {

    // Progress Bar
    remainder = new double (0.0);
    progWin = new QWidget();
    progLayout = new QGridLayout();
    progress = new QProgressBar();
    progLayout->addWidget(progress, 0, 0);
    progWin->setLayout(progLayout);
    progWin->resize(350, 0);
    progress->setRange(0, 1000000000);

    progWin->setFont(QFont("Serif", 12, QFont::Normal, false));



}


/***
Function: load_dose_data
------------------------
Process: Used to read a 3ddose file

Inputs: path: the absolute path of the 3ddose file
***/
void read_dose::load_dose_data_comparison(QString path) {

    // Open the .3ddose file
    QFile *file;
    QTextStream *input;
    file = new QFile(path);

    // Set up the progress bar
    create_progress_bar();
    progress->reset();
    *remainder = 0;
    progWin->setWindowTitle("Importing 3ddose File");
    progWin->show();
    progWin->activateWindow();
    progWin->raise();


    // Determine the increment size of the status bar this 3ddose file gets
    double increment = 1000000000;

    //Opening and reading the 3ddose file
    if (file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        input = new QTextStream(file);

        updateProgress(increment*0.005);

        // Read in the number of voxels
        *input >> x;
        *input >> y;
        *input >> z;

        cx.resize(x+1);
        cy.resize(y+1);
        cz.resize(z+1);
        val.resize(x*y*z);

        updateProgress(increment*0.01);

        // Read in boundaries
        for (int i = 0; i <= x; i++) {
            *input >> cx[i];
        }
        for (int j = 0; j <= y; j++) {
            *input >> cy[j];
        }
        for (int k = 0; k <= z; k++) {
            *input >> cz[k];
        }

        updateProgress(increment*0.01);


        // Resize increments so that the rest of the section of the progress
        // bar is divided by two times the number of z values, so that each
        // time the loops iterate througha new z, the progress bar updates

        increment *= 0.975;
        increment = increment/(2*z);


        // resize the 3D matrix to hold all dose values
        {
            QVector <double> vz(z, 0);
            QVector <QVector <double> > vy(y, vz);
            QVector <QVector <QVector <double> > > vx(x, vy);
            valMatrix = vx;
            QVector <double> ez(z, 0);
            QVector <QVector <double> > ey(y, ez);
            QVector <QVector <QVector <double> > > ex(x, ey);
            errMatrix = ex;
        }

        // Read in all the dose
        for (int k = 0; k < z; k++) {
            for (int j = 0; j < y; j++)
                for (int i = 0; i < x; i++) {
                    *input >> valMatrix[i][j][k];
                    if (i == 0 && j==0 && k==0) { //initialize the max, min dose vales
                        max_val = min_val = valMatrix[0][0][0];
                    }
                    if (valMatrix[i][j][k] > max_val) {
                        max_val = valMatrix[i][j][k];
                    }
                    if (valMatrix[i][j][k]< min_val) {
                        min_val = valMatrix[i][j][k];
                    }
                }
            updateProgress(increment);
        }

        // Read in all the error
        if (!input->atEnd()) {
            for (int k = 0; k < z; k++) {
                for (int j = 0; j < y; j++)
                    for (int i = 0; i < x; i++) {
                        *input >> errMatrix[i][j][k];
                    }
                updateProgress(increment);
            }
        }
        delete input;

    }

    delete file;


    progress->setValue(1000000000);
    progWin->hide();

    //Calculate some values
    x_thick = fabs(cx[0]-cx[1]);
    y_thick = fabs(cy[0]-cy[1]);
    z_thick = fabs(cz[0]-cz[1]);

    std::cout<<valMatrix.size() <<"  " <<valMatrix[0].size() <<" " <<valMatrix[0][0].size() <<"\n";
    if (cz[0] > cz[1]) { //if z-values in decreasing order, flip them
        flip = true;
    }



}

void read_dose::trim_dose(int xn, int yn, int zn, QString path_3ddose) {

    std::ofstream dose_file;
    dose_file.open(path_3ddose.toStdString().c_str());

    //Output the egsphant to a file
    //-----------------------------------
    if (dose_file.is_open()) {

        //updateProgress(increment);
        //1: Output the number of voxels in each dimension
        dose_file << xn << "   " << yn << "   " << zn <<"\n";

        //updateProgress(increment);

        //2,3,4: Output the voxel boundaries in the x,y,z direction
        for (int i=0; i< xn; i++) {
            dose_file<<" " <<cx[i];
        }

        dose_file <<"\n";


        for (int i=0; i<yn; i++) {
            dose_file<<" " <<cy[i];
        }

        dose_file<<"\n";


        for (int i=0; i<zn; i++) {
            dose_file<<" " <<cz[i];
        }

        dose_file<<"\n";


        //5: Dose value array
        for (int k = 0; k < zn; k++) {
            for (int j = 0; j < yn; j++)
                for (int i = 0; i < xn; i++) {
                    dose_file<< valMatrix[i][j][k];
                }
        }

        dose_file<<"\n";
        dose_file.close();

        std::cout<<"Successfully created the 3ddose file. Location: " <<path_3ddose.toStdString() <<"\n";


    }
    else {
        std::cout<<"ERROR: Couldn't open 3ddose file location \n";
    }

}




