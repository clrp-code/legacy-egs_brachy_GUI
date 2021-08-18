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

/***

Creates window to ask user to verify the tissue assignment scheme

***/
#include "tissue_check.h"

Tissue_check::Tissue_check(QWidget *parent, QVector<QString> dicom_struct_name, QVector<QString> organ_names, QVector<QString> dicom_struct_type)
    : QWidget(parent) {
    mom = parent;

    int j=0;
    for (int i=0; i<dicom_struct_name.size(); i++) {
        if (dicom_struct_type[i].toUpper() !=  "EXTERNAL") {
            dicom_struct.resize(dicom_struct.size()+1);
            dicom_struct[j] = dicom_struct_name[i];
            if (!dicom_struct_type[i].isEmpty()) {
                dicom_struct[j].append(" (" + dicom_struct_type[i].toLower() + ")");    //ordered list of the dicom contours, unmodified name from DICOM
            }
            j++;
        }
    }

    organs_unique = organ_names;                        //list of all the organ names in the TAS

    predicted_organ.resize(dicom_struct.size());
    predicted_organ.fill(0);

    //attempt to match dicom_contour to organ name
    for (int i=0; i<dicom_struct.size(); i++) {
        for (int j=0; j<organs_unique.size(); j++) {
            if (dicom_struct[i].contains(organs_unique[j],Qt::CaseInsensitive) || organs_unique[j].contains(dicom_struct[i],Qt::CaseInsensitive)) {
                predicted_organ[i] = j+1;
            }
        }
    }
    organs_unique.push_front(" "); //To allow for no TAS organ to be assiciated to the dicom structure

    createLayout();     // This creates the layout structure
    connectLayout();    // This connects the layout with the SLOTs

}


Tissue_check::~Tissue_check() {
    for (int i = 0; i < organs->size(); i++) {
        delete (*organs)[i];
    }
    organs->clear();

    for (int i = 0; i < dicom_cont->size(); i++) {
        delete (*dicom_cont)[i];
    }
    dicom_cont->clear();


    delete window;

}


void Tissue_check::createLayout() {
    window = new QWidget();

    tissLayout = new QGridLayout();
    QGroupBox *groupBox = new QGroupBox(tr("Verify the mapping from DICOM contour to tissue assignment scheme organ"));
    //Window pane: LHS is the name of the contour and the
    //             RHS is a combobox where you can select the correct organ, matched one is preslected

    QString blank_string = " ";

    //Creating and filling the contour and organ names
    organs = new QVector<QComboBox *>;
    dicom_cont = new QVector<QLabel *>;
    mask_selection = new QVector<QCheckBox *>;

    organs->resize(dicom_struct.size()+1);
    dicom_cont->resize(dicom_struct.size());
    mask_selection->resize(dicom_struct.size());

    QLabel *mask_title = new QLabel("Generate \n mask");
    tissLayout->addWidget(mask_title,1,2);

    for (int i=0; i<dicom_struct.size(); i++) {
        (*dicom_cont)[i]= new QLabel(dicom_struct[i]);
        (*dicom_cont)[i] -> setToolTip(tr("Name of contour in the dicom file"));
        tissLayout->addWidget((*dicom_cont)[i],i+2,0);

        (*organs)[i] = new QComboBox();
        (*organs)[i] ->setToolTip(tr("Verify the mapping"));
        (*organs)[i]->addItems(organs_unique.toList());
        (*organs)[i]->setCurrentIndex(predicted_organ[i]);
        tissLayout->addWidget((*organs)[i],i+2,1);

        (*mask_selection)[i] = new QCheckBox();
        tissLayout->addWidget((*mask_selection)[i],i+2,2);

    }

    ;
    image_selection = new QCheckBox();
    image_selection->setText("Generate medium and \n density images");
    image_selection -> setToolTip(tr("Select to generate figures (.png) \nof the media and desnity for \neach slize of the patient"));
    tissLayout->addWidget(image_selection,dicom_struct.size()+4,0);

    groupBox->setLayout(tissLayout);
    QLabel *title = new QLabel(tr("Verify the mapping between each contour in the DICOM data\n") +
                               tr("to the corresponding organ in the tissue assignment scheme\n"));
    apply = new QPushButton(tr("Save the mapping"));
    cancel = new QPushButton(tr("Cancel"));
    int pos = dicom_struct.size() +5;

    //Adding all the elements
    tissLayout->addWidget(title,0,0,1,3);
    //tissLayout->addWidget(groupBox ,1,0,1,1); //issue here adding qgroupbox to qgridlayout
    tissLayout->addWidget(apply,pos,2,1,1);
    tissLayout->addWidget(cancel,pos,3,1,1);
    tissLayout->setColumnStretch(0, 2);
    tissLayout->setColumnStretch(1, 2);
    window->setLayout(tissLayout);
    window->setWindowTitle(tr("Verify the mapping"));

    scrollArea = new QScrollArea();
    scrollArea->setWidget(window);

    scrollArea->viewport()->setAutoFillBackground(true);
    scrollArea->setWindowTitle(QObject::tr("Verify the mapping"));

    scrollArea->show();

    int resize_length = 37*(dicom_struct.size()+1+0.5); //used to potentially resize the window
    if (dicom_struct.size() >=15) {
        resize_length = 550;
    }

    scrollArea->resize(575, resize_length);
}

void Tissue_check::connectLayout() {

    connect(apply, SIGNAL(clicked()),
            this, SLOT(close_tissue_check()));

    // connect(apply, SIGNAL(clicked()),
    // scrollArea, SLOT(quit()));

    // connect(apply, SIGNAL(clicked()),
    // window, SLOT(quit()));

    connect(cancel, SIGNAL(clicked()),
            this, SLOT(cancel_window()));

}

/***
Function: closeEvent
***/
void Tissue_check::closeEvent(QCloseEvent *event) {
    event->ignore();

}


/***
Function: close_tissue_check
***/
void Tissue_check::close_tissue_check() {
    generateMask.resize(dicom_struct.size());
    generateMask.fill(false);
    selected_tas_name.resize(dicom_struct.size());

    for (int i=0; i<dicom_struct.size(); i++) {
        selected_tas_name[i] = (*organs)[i]->currentText();
        if ((*mask_selection)[i]->isChecked()) {
            generateMask[i] = true;
        }
    }

    if (image_selection->isChecked()) {
        generateImages = true;
    }

    closed_check = true;

    this->setDisabled(TRUE);
    mom->setEnabled(TRUE);
    window->hide();
    scrollArea->hide();
    QApplication::processEvents();
    emit closed();

}

/***
Function: cancel_window
***/
void Tissue_check::cancel_window() {

    this->setDisabled(TRUE);
    mom->setEnabled(TRUE);
    window->hide();
    scrollArea->hide();

    emit exit();

}










