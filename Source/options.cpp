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

#include "options.h"

/***

This class creates the advanced options window where the user can specify
changes to the egsinp file. Including the ability to:
    -change the default number of histories
    -choose to score the energy deposition
    -changed the default muen file

    //Future additions
        //user decision on if to create a wrapper for seeds (es in peppavreasthdr egsinp file)

***/

Options::Options(QWidget *parent, QString egs_home)
    : QWidget(parent) { // Pass the parameter to the QWidget constructor
    mom = parent;

    muen_home = egs_home + "/egs_home/egs_brachy/lib/muen/";
    egs_brachy_home_path = egs_home;

    //get the list of transport parameter files
    //-----------------------------------------------------
    QString transport_path = egs_brachy_home_path + "/lib/transport/";
    QDir dir_transp(transport_path);
    QFileInfoList fileName_transp = dir_transp.entryInfoList();

    for (int i=0; i < fileName_transp.size(); i ++) {
        QFileInfo fileInfo_transp = fileName_transp.at(i);
        if (fileInfo_transp.isFile()&& !fileInfo_transp.fileName().contains("README")) {
            transport_names.append(fileInfo_transp.fileName());
            transport_names_path.append(fileInfo_transp.absoluteFilePath());
        }
    }

    createLayout(); // This creates the layout structure
    connectLayout(); // This connects the layout with the SLOTs
    //window->resize(1000, 600);
}


Options::~Options() {


}




void Options::createLayout() {
    window = new QWidget();

    //Change the number of histories
    QGroupBox *number_histories_box = new QGroupBox(tr("Change the default number of histories"));

    numb_histories = new QLineEdit();
    numb_histories->setText("1e8");
    QGridLayout *histories_Layout = new QGridLayout();
    histories_Layout->addWidget(numb_histories, 1, 0, 1, 2);

    number_histories_box->setLayout(histories_Layout);

    QGroupBox *number_batch_box = new QGroupBox(tr("Change the default number of batches"));
    numb_batch = new QLineEdit();
    numb_batch->setText("1");
    QGridLayout *batch_Layout = new QGridLayout();
    batch_Layout->addWidget(numb_batch, 1, 0, 1, 2);
    number_batch_box->setLayout(batch_Layout);

    QGroupBox *number_chunk_box = new QGroupBox(tr("Change the default number of chunks"));
    numb_chunk = new QLineEdit();
    numb_chunk->setText("10");
    QGridLayout *chunk_Layout = new QGridLayout();
    chunk_Layout->addWidget(numb_chunk, 1, 0, 1, 2);
    number_chunk_box->setLayout(chunk_Layout);

    //Change score energy deposition
    QGroupBox *energy_deposition_box = new QGroupBox(tr("Score energy deposition"));
    energy_deposition_box->setToolTip(tr("The dafault value is no. \n") +
                                      tr("Score energy deposition is used to \n") +
                                      tr("score energy deposited by electrons.")); //also less efficient
    score_yes = new QRadioButton(tr("Yes"));
    QVBoxLayout *check_box = new QVBoxLayout();
    check_box->addWidget(score_yes);
    energy_deposition_box->setLayout(check_box);


    //Change the default muen file
    QGroupBox *muen_box = new QGroupBox(tr("Change the default muen file"));
    LoadButton = new QPushButton("Load a new muen file");
    QGridLayout *muenLayout = new QGridLayout();
    muenLayout->addWidget(LoadButton, 1, 0, 1, 2);
    muen_box->setLayout(muenLayout);
    muen_box->setToolTip(tr("The default muen file is the recommended file from\n") +
                         tr("the 'README' file in the egs_brachy muen folder.   "));

    //Change the default muen file
    QGroupBox *material_box = new QGroupBox(tr("Change the default material data file"));
    LoadMatButton = new QPushButton("Load a new material data file");
    QGridLayout *materialLayout = new QGridLayout();
    materialLayout->addWidget(LoadMatButton, 1, 0, 1, 2);
    material_box->setLayout(materialLayout);
    material_box->setToolTip(tr("The default file is material.dat file \n") +
                             tr("in egs_brachy.   "));



    //Apply Buttonn and Cancel Button
    cancel = new QPushButton("Cancel");
    apply = new QPushButton("Apply");

    QGridLayout *ExitLayout = new QGridLayout();
    ExitLayout->addWidget(cancel, 0, 0); //, 1, 2); //FIX this spacing
    ExitLayout->addWidget(apply, 0, 2); //, 3, 4);
    QGroupBox *ExitFrame = new QGroupBox();
    ExitFrame->setLayout(ExitLayout);

    transport_selection = new QListWidget();
    transport_selection->addItems(transport_names);
    transport_selection->setSelectionMode(QAbstractItemView::SingleSelection);

    transportBox = new QGroupBox(tr("Change the default transport parameter file by selecting an item from the list below"));
    transportBox->setToolTip(tr("The dafault file is chosen based on the     \n") +
                             tr("treatment type attribute in the DICOM files \n") +
                             tr("which identifies if the treatment is LDR/HDR.")); //also less efficient
    QVBoxLayout *sbox6 = new QVBoxLayout();
    sbox6->addWidget(transport_selection,0,0);
    transportBox->setLayout(sbox6);

    QGridLayout *options_layout = new QGridLayout();

    options_layout->addWidget(number_histories_box, 1, 0, 1, 1);
    options_layout->addWidget(number_batch_box, 2, 0, 1, 1);
    options_layout->addWidget(number_chunk_box, 3, 0, 1, 1);
    options_layout->addWidget(energy_deposition_box, 4, 0, 1, 1);
    options_layout->addWidget(muen_box, 5, 0, 1, 1);
    options_layout->addWidget(material_box, 6, 0, 1, 1);
    options_layout->addWidget(transportBox, 7, 0);
    options_layout->addWidget(ExitFrame,  8, 3, 1, 1);

    options_layout->setColumnStretch(0, 2);
    options_layout->setColumnStretch(1, 1);


    window-> setLayout(options_layout); //this may be writing over the 'main' window
    window->setHidden(TRUE);
    window->setWindowTitle(tr("Advanced Options"));

    save_options_on_startup("");
}



void Options::connectLayout() {

    connect(LoadButton, SIGNAL(clicked()),
            this, SLOT(load_muen()));

    connect(cancel, SIGNAL(clicked()),
            this, SLOT(close_options()));

    connect(apply, SIGNAL(clicked()),
            this, SLOT(set_values()));

    connect(LoadMatButton, SIGNAL(clicked()),
            this, SLOT(load_material()));



}





/***
Functon: set_values
-------------------
Used to save the user changes

***/
void Options::set_values() {

    number_hist = numb_histories->text().toDouble();
    number_batch = numb_batch->text().toDouble();
    number_chunk = numb_chunk->text().toDouble();

    checked_score_energy_deposition = false;  //set the default value

    if ((number_hist < 1) && (!numb_histories->text().isEmpty())) { //ERROR, must be greater than one, user can try again
        QMessageBox msgBox;
        msgBox.setText(tr("The number of histories must be a positive integer."));
        msgBox.setWindowTitle(tr("Error"));
        msgBox.exec();

    }
    else if ((number_batch < 1) && (!numb_batch->text().isEmpty())) { //ERROR, must be greater than one, user can try again
        QMessageBox msgBox;
        msgBox.setText(tr("The number of batches must be a positive integer."));
        msgBox.setWindowTitle(tr("Error"));
        msgBox.exec();

    }
    else if ((number_chunk < 1) && (!numb_chunk->text().isEmpty())) { //ERROR, must be greater than one, user can try again
        QMessageBox msgBox;
        msgBox.setText(tr("The number of chunks must be a positive integer."));
        msgBox.setWindowTitle(tr("Error"));
        msgBox.exec();

    }
    else {

        if (score_yes->isChecked()) {
            checked_score_energy_deposition = true;
        }

        if (transport_selection->currentRow() != -1) {
            transport_param_path = transport_names_path[transport_selection->currentRow()];
        }

        //closing the window
        this->setDisabled(TRUE);
        mom->setEnabled(TRUE);
        window->hide();
    }

    //Modify the advanced options in the egsinp class
    QFile egsinp(egsinp_path);
    if (egsinp.exists()) {
        QTextStream out(stdout);
        QFile muen(muen_file);
        QFile material(material_file);
        QFile transp_param(transport_param_path);
        QFile temp(egsinp_path+"temp");

        if (!egsinp.open(QIODevice::ReadOnly | QIODevice::Text) || !temp.open(QIODevice::WriteOnly | QIODevice::Text)) {
            out<<"Unable to edit advanced options settings in " <<egsinp.fileName() <<endl;
        }
        else {

            // Open new file to write
            QTextStream in(&egsinp);
            QTextStream out_file(&temp);


            while (!in.atEnd()) {
                QString line = in.readLine();

                if (line.contains("	ncase = ")) {                           //change number of histories
                    out_file <<"	ncase = " <<number_hist <<endl;
                }
                else if (line.contains("	nbatch = ")) {                             //change number of batches
                    out_file <<"	nbatch = " <<number_batch <<endl;
                }
                else if (line.contains("	nchunk = ")) {                             //change number of chunks
                    out_file <<"	nchunk = " <<number_chunk <<endl;
                }
                else if (line.contains("    score energy deposition = ")) { //change score energy deopsition

                    if (checked_score_energy_deposition) {
                        out_file <<"    score energy deposition = yes" <<endl;
                    }
                    else {
                        out_file <<"    score energy deposition = no" <<endl;
                    }

                }
                else if (line.contains("    muen file = ") && muen.exists()) {  //change muen
                    out_file <<"    muen file = " <<muen_file <<endl;
                }
                else if (line.contains("    material data file = ") && material.exists()) { //change material
                    out_file <<"    material data file = " <<material_file <<endl;
                }
                else if (line.contains("        density file = ") && material.exists()) {
                    out_file <<"        density file = " <<material_file <<endl;
                }
                else if (line.contains("/egs_home/egs_brachy/lib/transport/") && transp_param.exists()) { //change transpor param
                    out_file <<"include file = " <<transport_param_path <<endl;
                }
                else {                                                      //do nothing
                    out_file <<line <<"\n";
                }

            }
            temp.close();
            egsinp.close();

            //deleting the old egsinp and replacing it with the updated file
            egsinp.remove();
            temp.rename(egsinp_path);


        }
    }
    emit(closed());

}


/***
Function: load_muen
-------------------
Asks the user to select the muen file to upload
***/
void Options::load_muen() {
    muen_file = QFileDialog::getOpenFileName(
                    this,
                    tr("Select a muen file to open"),
                    muen_home,
                    "muen (*.muendat)");
}

void Options::load_material() {
    material_file = QFileDialog::getOpenFileName(
                        this,
                        tr("Select a material data file to open"),
                        egs_brachy_home_path,
                        "dat (*.dat)");


}


/***
Function: close_options
-----------------------
The fields are returned to their original values when the user clicked the 'close' button

***/
void Options::close_options() {

    this->setDisabled(TRUE);
    mom->setEnabled(TRUE);
    window->hide();

    //Reset options if they were changed
    numb_histories->setText(QString::number(number_hist_startup));
    numb_batch->setText(QString::number(number_batch_startup));
    numb_chunk->setText(QString::number(number_chunk_startup));

    if (checked_energy_startup) {
        score_yes->setChecked(true);    //checked
    }
    else {
        score_yes->setChecked(false);    //not checked
    }

    muen_file = muen_on_startup;
    material_file = material_on_startup;
    transport_selection->setCurrentRow(transp_index_startup);
    emit(closed());
}

/***
Function: save_options_on_startup
--------------------------------
Process: This functions saves the parameters when the window is opened
         Parameters are used if cancel is pressed
***/
void Options::save_options_on_startup(QString egsinp) {
    egsinp_path = egsinp;

    number_hist_startup = numb_histories->text().toDouble();
    number_batch_startup = numb_batch->text().toDouble();
    number_chunk_startup = numb_chunk->text().toDouble();

    if (score_yes->isChecked()) {
        checked_energy_startup = true;
    }
    else {
        checked_energy_startup = false;
    }

    muen_on_startup = muen_file;
    material_on_startup = material_file;
    transp_index_startup = transport_selection->currentRow();

}

void Options::closeEvent(QCloseEvent *event) {
    QTextStream out(stdout);
    out<<"in close event" <<endl;
    // mom->setEnabled(TRUE);
    // window->hide();
    // QApplication::processEvents();
    //emit closed();
    emit(closed());

    event->ignore();

}



