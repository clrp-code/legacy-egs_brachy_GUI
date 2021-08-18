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

#include "file_selector.h"

/*

This class is used to create the window to change the egs_brachy input file names & location and to add
a header to the egsinput file

*/

/***
Function: File_selector
-----------------------
Creates an instance of the file_selector
***/
File_selector::File_selector(QWidget *parent, QString EGSHOME, QString egsinput, QString egsphant, QString transf)
    : QWidget(parent) { // Pass the parameter to the QWidget constructor
    mom = parent;

    home_path = EGSHOME;
    egsinp_default = egsinput;
    egsphant_default = egsphant;
    transf_default = transf;

    createLayout(); // This creates the layout structure
    connectLayout(); // This connects the layout with the SLOTs

}



File_selector::~File_selector() {
    //delete

}


/***
Function: createLayout
----------------------
Process: this file is used to setup and create the window

***/
void File_selector::createLayout() {

    window = new QWidget();
    File_selector_layout = new QGridLayout();

    description = new QPlainTextEdit();
    egs_header = new QLabel(tr("Add an egsinp header to the field below:"));

    QString cStyle = "QLabel{background-color: rgb(242, 242, 242);";
    cStyle += "border-top: 1px solid black;";
    cStyle += "border-left: 1px solid black;";
    cStyle += "border-right: 1px solid rgb(160, 160, 160);";
    cStyle += "border-bottom: 1px solid rgb(160, 160, 160);";
    cStyle += "}";


    select1 = new QPushButton(tr("Browse..."));
    select2 = new QPushButton(tr("Browse..."));
    select3 = new QPushButton(tr("Browse..."));


    apply = new QPushButton(tr("Ok"));
    cancel = new QPushButton(tr("Cancel"));

    egsphant_path = new QLabel();
    egsinp_path = new QLabel();
    transformation_path = new QLabel();


    //creating the egsphant box
    if (!egsphant_default.isEmpty()) {
        egsphant_location = new QLabel(tr("Location of egsphant file"));
        egsphant_path->setStyleSheet(cStyle);
        egsphant_path->setToolTip(tr("The default file name is created using the date time."));
        egsphant = new QHBoxLayout();
        egsphant->addWidget(egsphant_path);
        egsphant->addWidget(select1);
        QVBoxLayout *vbox1 = new QVBoxLayout;
        vbox1->addWidget(egsphant_location);
        vbox1->addLayout(egsphant);
        phantFrame = new QGroupBox();
        phantFrame->setLayout(vbox1);
        File_selector_layout->addWidget(phantFrame);
    }



    //creating the seed transformaton box
    if (!transf_default.isEmpty()) {
        transformation_location = new QLabel(tr("Location of seed transformation file "));
        transformation_path->setStyleSheet(cStyle);
        transformation_path->setToolTip(tr("The default file name is created using the date time."));
        transf = new QHBoxLayout();
        transf->addWidget(transformation_path);
        transf->addWidget(select3);
        QVBoxLayout *vbox3 = new QVBoxLayout;
        vbox3->addWidget(transformation_location);
        vbox3->addLayout(transf);
        transfFrame = new QGroupBox();
        transfFrame->setLayout(vbox3);
        File_selector_layout->addWidget(transfFrame);
    }


    //creating the egsinp box
    if (!egsinp_default.isEmpty()) {
        egsinp_location = new QLabel(tr("Location of egsinp file "));
        egsinp_path->setStyleSheet(cStyle);
        egsinp_path->setToolTip(tr("The default file name is created using the date time."));
        egsinp = new QHBoxLayout();
        egsinp->addWidget(egsinp_path);
        egsinp->addWidget(select2);
        QVBoxLayout *vbox2 = new QVBoxLayout;
        vbox2->addWidget(egsinp_location);
        vbox2->addLayout(egsinp);
        inpFrame = new QGroupBox();
        inpFrame->setLayout(vbox2);
        File_selector_layout->addWidget(inpFrame);


        //creating the add header box
        QVBoxLayout *vbox4 = new QVBoxLayout;
        vbox4->addWidget(egs_header);
        vbox4->addWidget(description);
        headerFrame = new QGroupBox();
        headerFrame->setLayout(vbox4);

        egs_header->setToolTip(tr("The header will be displayed as a comment at the \n")+
                               tr("top of the egsinp file."));

        File_selector_layout->addWidget(headerFrame);
    }

    File_selector_layout->addWidget(apply, 5, 2);
    File_selector_layout->addWidget(cancel, 5, 3);

    File_selector_layout->setColumnStretch(0, 3);
    File_selector_layout->setColumnStretch(1, 3);
    File_selector_layout->setColumnStretch(2, 3);


    window -> setLayout(File_selector_layout); //this may be writing over the 'main' window
    window->setHidden(TRUE);
    window->setWindowTitle(tr("Change Input File Locations and Settings"));

    //On first time opening window, set text to default
    if (changed_location == false) {

        egsphant_path->setText(egsphant_default);
        egsinp_path->setText(egsinp_default);
        transformation_path->setText(transf_default);


    }


    description->setPlainText(egsinp_header);

    save_info_startup();

}

void File_selector::connectLayout() {
    connect(select1, SIGNAL(clicked()),
            this, SLOT(change_egsphant()));

    connect(select2, SIGNAL(clicked()),
            this, SLOT(change_egsinp()));

    connect(select3, SIGNAL(clicked()),
            this, SLOT(change_transformation()));

    connect(cancel, SIGNAL(clicked()),
            this, SLOT(close_file_selector()));
    connect(apply, SIGNAL(clicked()),
            this, SLOT(save_file_selector()));

}




/***
Function: change_egsinp
-----------------------
Process: function is called when user wants to change the egsinp file location/name
            If the egsinp has been created, it will be renamed
***/
void File_selector::change_egsinp() {
    QString existing_valid = egsinp_path->text();

    QFile egsinp(egsinp_path->text());

    if (egsinp.exists()) {  //rename the existing egsinp file
        QTextStream out(stdout);

        QString file = QFileDialog::getSaveFileName(
                           this,
                           tr("Rename the egsinp file"),
                           egsinp_path->text(),
                           tr("egsinp"));

        if (!file.isEmpty()) {
            check_valid_egsinp(file, existing_valid);

            egsinp_path->setText(file);

            //Checking if the new file name is an existing file
            QFile new_file_name(file);
            if (new_file_name.exists()) {
                new_file_name.remove();
            }

            egsinp.rename(file); //renaming the transformation file

            out<<"egsinp file renamed from " <<egsinp.fileName();
            out<<" to " <<file <<endl;

        }

    }
    else {
        QString file = QFileDialog::getSaveFileName(
                           this,
                           tr("Save egsinp as"),
                           egsinp_path->text(),
                           tr("egsinp"));
        if (!file.isEmpty()) {
            check_valid_egsinp(file, existing_valid);

        }
    }
}


/***
Function: check_valid_egsinp
----------------------------
Process: Check that the file is in a valid location (in the egs_brachy folder)

***/
void File_selector::check_valid_egsinp(QString file, QString existing_valid) {
    if (!file.endsWith(".egsinp")) {
        file += ".egsinp";
    }

    egsinp_path->setText(file);

    //the egsinp file must be within the egs_brachy folder
    if (!egsinp_path->text().contains(home_path)) {


        QMessageBox msgBox;
        msgBox.setText(tr("Error: The egsinp file must be within \n") + home_path);
        msgBox.exec();


        QString file = QFileDialog::getSaveFileName(
                           this,
                           tr("Save egsinp as"),
                           existing_valid,
                           tr("egsinp"));

        if (!file.isEmpty()) {
            if (!file.endsWith(".egsinp")) {
                file += ".egsinp";
            }

            egsinp_path->setText(file);
        }

    }
}

/***
Function: change_egsphant
-----------------------
Process: function is called when user wants to change the egsphant file location/name
            If the egsphant file already exists, the egsinp file is updated
            with the new path
***/
void File_selector::change_egsphant() {

    QFile egsphant_gzip(egsphant_path->text() + ".gz");
    QFile egsphant(egsphant_path->text());
    if (egsphant_gzip.exists()) { //rename the existing gzipped file
        QTextStream out(stdout);

        QString file = QFileDialog::getSaveFileName(
                           this,
                           tr("Rename the egsphant file"),
                           egsphant_path->text(),
                           tr("egsphant (*.egsphant)"));

        if (!file.isEmpty()) {
            egsphant_path->setText(file);

            //Checking if the new file name is an existing file
            QFile new_file_name(file);
            if (new_file_name.exists()) {
                new_file_name.remove();
            }

            out<<"egsphant file renamed from " <<egsphant_gzip.fileName();

            egsphant_gzip.rename(file + ".gz"); //renaming the transformation file


            out<<" to " <<egsphant_gzip.fileName()<<endl;

            //Need to edit the egsinp file with the new egsphant name
            QFile egsinp(egsinp_path->text());
            QFile temp(egsinp_path->text()+"temp");

            if (!egsinp.open(QIODevice::ReadOnly | QIODevice::Text) || !temp.open(QIODevice::WriteOnly | QIODevice::Text)) {
                out<<"Unable to edit reference to egsphant file in " <<egsinp.fileName() <<endl;
            }
            else {

                // Open new file to write
                QTextStream in(&egsinp);
                QTextStream out_file(&temp);

                //new line to replace exisitng in the egsinp
                QString new_file = "        egsphant file = " + file + ".gz\n";

                while (!in.atEnd()) {
                    QString line = in.readLine();

                    if (line.contains("egsphant file =")) {
                        out_file <<new_file;
                    }
                    else {
                        out_file <<line <<"\n";
                    }

                }
                temp.close();
                egsinp.close();

                //deleting the old egsinp and replacing it with the updated file
                egsinp.remove();
                temp.rename(egsinp_path->text());


            }


        }

    }
    else if (egsphant.exists()) {   //rename the existing file (not gzipped)
        QTextStream out(stdout);

        QString file = QFileDialog::getSaveFileName(
                           this,
                           tr("Rename the egsphant file"),
                           egsphant_path->text(),
                           tr("egsphant (*.egsphant)"));

        if (!file.isEmpty()) {
            egsphant_path->setText(file);

            //Checking if the new file name is an existing file
            QFile new_file_name(file);
            if (new_file_name.exists()) {
                new_file_name.remove();
            }

            out<<"egsphant file renamed from " <<egsphant.fileName();

            egsphant.rename(file); //renaming the transformation file

            out<<" to " <<file <<endl;

            //Need to edit the egsinp file with the new egsphant name
            QFile egsinp(egsinp_path->text());
            QFile temp(egsinp_path->text()+"temp");

            if (!egsinp.open(QIODevice::ReadOnly | QIODevice::Text) || !temp.open(QIODevice::WriteOnly | QIODevice::Text)) {
                out<<"Unable to edit reference to egsphant file in " <<egsinp.fileName() <<endl;
            }
            else {

                // Open new file to write
                QTextStream in(&egsinp);
                QTextStream out_file(&temp);

                //new line to replace exisitng in the egsinp
                QString new_file = "        egsphant file = " + file + "\n";

                while (!in.atEnd()) {
                    QString line = in.readLine();

                    if (line.contains("egsphant file =")) {
                        out_file <<new_file;
                    }
                    else {
                        out_file <<line <<"\n";
                    }

                }
                temp.close();
                egsinp.close();

                //deleting the old egsinp and replacing it with the updated file
                egsinp.remove();
                temp.rename(egsinp_path->text());


            }

        }

    }
    else {
        QString file = QFileDialog::getSaveFileName(
                           this,
                           tr("Save egsphant as"),
                           egsphant_path->text(),
                           tr("egsphant (*.egsphant)"));

        if (!file.isEmpty()) {
            egsphant_path->setText(file);
        }

    }
}


/***
Function: change_transformation
-----------------------
Process: function is called when user wants to change the transformation file location/name
            If the file has already been created, the file name is changed and the egsinp
            references to the file names are changed aswell
***/
void File_selector::change_transformation() {
    QFile transform(transformation_path->text());

    if (transform.exists()) { //rename the existing file

        QTextStream out(stdout);

        QString file = QFileDialog::getSaveFileName(
                           this,
                           tr("Rename the seed transformation file"),
                           transformation_path->text(),
                           tr("transformation file"));
        if (!file.isEmpty()) {
            transformation_path->setText(file);

            //Checking if the new file name is an existing file
            QFile new_file_name(file);
            if (new_file_name.exists()) {
                new_file_name.remove();
            }

            out<<"Seed transformation file renamed from " <<transform.fileName();

            transform.rename(file); //renaming the transformation file


            out<<" to " <<file <<endl;

            //Need to edit the egsinp file with the new seed transfomration name
            //------------------------------------------------------------------
            QFile egsinp(egsinp_path->text());
            QFile temp(egsinp_path->text()+"temp");

            if (!egsinp.open(QIODevice::ReadOnly | QIODevice::Text) || !temp.open(QIODevice::WriteOnly | QIODevice::Text)) {
                out<<"Unable to edit reference to seed transformation file in " <<egsinp.fileName() <<endl;
            }
            else {

                // Open new file to write
                QTextStream in(&egsinp);
                QTextStream out_file(&temp);

                //new line to replace exisitng in the egsinp
                QString new_file = "        include file = " + file +"\n";

                while (!in.atEnd()) {
                    QString line = in.readLine();

                    if (line.contains(":start transformations:")) {
                        out_file <<line <<"\n";
                        QString line = in.readLine();
                        out_file <<new_file;

                    }
                    else {
                        out_file <<line <<"\n";
                    }
                }
                temp.close();
                egsinp.close();

                //deleting the old egsinp and replacing it with the updated file
                egsinp.remove();
                temp.rename(egsinp_path->text());


            }


        }

    }
    else {          //Just changing hte name
        QString file = QFileDialog::getSaveFileName(
                           this,
                           tr("Save seed transformation file as"),
                           transformation_path->text(),
                           tr("transformation file"));

        if (!file.isEmpty()) {
            transformation_path->setText(file);
        }
    }
}

/***
Function: save_file_selector
----------------------------
Process: This function is used to save the changes made in the window
         All values are replaced with the new values upon clicking apply


***/
void File_selector::save_file_selector() {
    changed_location = true;

    //saving the header
    egsinp_header = description->toPlainText();
    egsinp_header.replace('\n', "\n###"); //Adding a # at start of each line, is a comment in egsinp

    //if the egsinp file exists, the header needs to be addded
    QFile egsinp(egsinp_path->text());
    QTextStream out(stdout);

    if (egsinp.exists()) {  //add the header to egsinp file and remove the existing header
        QFile temp(egsinp_path->text()+"temp");

        if (!egsinp.open(QIODevice::ReadOnly | QIODevice::Text) || !temp.open(QIODevice::WriteOnly | QIODevice::Text)) {
            out<<"Unable to edit header in " <<egsinp.fileName() <<endl;
        }
        else {

            // Open new file to write
            QTextStream in(&egsinp);
            QTextStream out_file(&temp);

            //reads the egsinp file and replaces the current header
            //with the new header
            while (!in.atEnd()) {
                QString line = in.readLine();

                if (line.contains("#####  This is an automatically generated Patient Input File")) {

                    QString line = in.readLine();

                    while (!line.contains(":start run control:")) { //skipping exisitng header
                        line = in.readLine();
                        //out<<"while " <<line <<endl;
                    }
                    out_file << "#####" <<egsinp_header <<endl;
                    out_file << "####################################################################################################" <<endl;
                    out_file <<line <<"\n";


                }
                else {
                    out_file <<line <<"\n";
                }
            }
            temp.close();
            egsinp.close();

            //deleting the old egsinp and replacing it with the updated file
            egsinp.remove();
            temp.rename(egsinp_path->text());

        }
    }

    this->setDisabled(TRUE);
    mom->setEnabled(TRUE);
    window->hide();

    emit closed();
}

/***
Function: close_file_selector
-----------------------------
Process: This function s called when the cancel button is clicked
        All changes made are not implemented and all values are reverted
        to what they were upon opening the window

***/
void File_selector::close_file_selector() {
    //Restore values from before opening window
    egsphant_path->setText(initial_egsphant);
    egsinp_path->setText(initial_egsinp);
    transformation_path->setText(initial_transformation);
    egsinp_header = initial_egsinp_header;



    this->setDisabled(TRUE);
    mom->setEnabled(TRUE);
    window->hide();

    emit cancel_pressed();

}

/***
Function: save_info_startup
---------------------------
Process: This function is used to save the text values when the window is first opened
***/
void File_selector::save_info_startup() {

    //Upon opening the window, save the text values (if users clicks cancel after making changes)
    initial_egsphant = egsphant_path->text();
    initial_egsinp = egsinp_path->text();
    initial_transformation = transformation_path->text();
    initial_egsinp_header = egsinp_header;

}





















