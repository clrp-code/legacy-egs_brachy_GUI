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

#include "priority.h"


priority::~priority() {

    delete window;

}

priority::priority(QWidget *parent, QVector<QString> structName, QVector<bool> structUnique)
    : QWidget(parent) {
    mom = parent;

    //If structInclude is false, don't include in the list
    //structInclude = structUnique;

    QMap <QString, int> structMapPrio; //QString key, int value
    Names = structName;

    //Assign a default priority
    for (int i = 0; i < structName.size(); i++) {
        structOrder.insert(i,structOrder[i]); //Save the initial order of the structNames
        if (structUnique[i] == true) {
            structMapPrio.insert(structName[i], structName.size()-i+1);
        }
    }

    //Set some default priorities
    if (structMapPrio.contains("PTV")) {
        structMapPrio.insert("PTV", 1000);
    }
    if (structMapPrio.contains("PTV 0.5")) {
        structMapPrio.insert("PTV 0.5", 1001);
    }
    if (structMapPrio.contains("PTV 1.0")) {
        structMapPrio.insert("PTV 1.0", 1000);
    }
    if (structMapPrio.contains("CTV")) {
        structMapPrio.insert("CTV", 999);
    }

    //Read the default priority file
    QString tempS;
    QString TAS_tag("Default");
    QString priority("Assignment_Schemes/"+TAS_tag+"_priority.txt");
    QFile *file = new QFile(priority);
    if (file->open(QIODevice::ReadOnly | QIODevice::Text) && structName.size() > 0) {
        QTextStream input(file);

        while (!input.atEnd()) {
            tempS = input.readLine();
            QString tempS2 = "";

            if (tempS.contains(" ")) {
                tempS2 = tempS.split(' ', QString::SkipEmptyParts)[0];
                tempS2.replace(QString("_"),QString(" "));

                if (structMapPrio.contains(tempS2)) {
                    structMapPrio.insert(tempS2, tempS.split(' ', QString::SkipEmptyParts)[1].toInt());
                }
            }
            else if (tempS.contains("\t")) {
                tempS2 = tempS.split('\t', QString::SkipEmptyParts)[0];
                tempS2.replace(QString("_"),QString(" "));

                if (structMapPrio.contains(tempS2)) {
                    structMapPrio.insert(tempS2, tempS.split('\t', QString::SkipEmptyParts)[1].toInt());
                }

            }
        }
    }
    delete file;

    QMap <int, QString> structOrder;

    QMap<QString, int>::const_iterator i = structMapPrio.constBegin();
    while (i != structMapPrio.constEnd()) {
        //std::cout << i.key().toStdString() << ": " << i.value() << "\n";
        structOrder.insert(i.value(), i.key());
        ++i;
    }

    QMap<int, QString>::const_iterator j = structOrder.constBegin();
    while (j != structOrder.constEnd()) {
        orderedNames.prepend(j.value());
        ++j;
    }

    createLayout();     // This creates the layout structure
    connectLayout();    // This connects the layout with the SLOTs

}



void priority::createLayout() {
    window = new QWidget();

    tissLayout = new QGridLayout();
    //QGroupBox *groupBox = new QGroupBox(tr(""));
    //Window pane: LHS is the name of the contour and the
    //             RHS is a combobox where you can select the correct organ, matched one is preslected

    prio_selection = new QListWidget();
    prio_selection->addItems(orderedNames);
    prio_selection->setDragDropMode(QAbstractItemView::InternalMove);
    //prio_selection->setSelectionMode(QAbstractItemView::SingleSelection);
    prio_selection->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    prio_selection->setToolTip(tr("Drag the contour to the desired position"));

    //groupBox->setLayout(tissLayout);
    QLabel *title = new QLabel(tr("Verify the priority of each contour in the DICOM data.\n") +
                               tr("Contours are ordered from highest to lowest priority\n"));
    apply = new QPushButton(tr("Save the priority order"));
    cancel = new QPushButton(tr("Cancel"));
    int pos = Names.size() +2;

    //Adding all the elements
    tissLayout->addWidget(title,0,0,1,3);
    tissLayout->addWidget(prio_selection,1,0,1,1);
    tissLayout->addWidget(apply,pos,2,1,1);
    tissLayout->addWidget(cancel,pos,3,1,1);
    tissLayout->setColumnStretch(0, 2);
    tissLayout->setColumnStretch(1, 2);
    window->setLayout(tissLayout);
    window->setWindowTitle(tr("Verify the mapping"));

    scrollArea = new QScrollArea();
    scrollArea->setWidget(window);


    scrollArea->viewport()->setAutoFillBackground(true);
    scrollArea->setWindowTitle(QObject::tr("Verify the contour priority"));

    scrollArea->show();

    int resize_length = 37*(Names.size()+1+0.5); //used to potentially resize the window
    if (Names.size() >=15) {
        resize_length = 550;
    }

    scrollArea->resize(575, resize_length);
}

void priority::connectLayout() {

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
void priority::closeEvent(QCloseEvent *event) {
    event->ignore();

}


/***
Function: close_tissue_check
***/
void priority::close_tissue_check() {
    //Itterate through the list, and assign a priority based on which is first
    //Match the priority vector with the order of the structName vector
    structPrio.resize(Names.size());
    structPrio.fill(0);

    for (int i=0; i<orderedNames.size(); i++) {
        //std::cout<<prio_selection->item(i)->text().toStdString() <<" " << orderedNames.size()+1-i <<"\n";
        for (int j=0; j<Names.size(); j++) {
            if (prio_selection->item(i)->text() == Names[j]) {
                structPrio[j] = orderedNames.size()+1-i;
            }
        }
    }

    closed_check = true;

    this->setDisabled(TRUE);
    mom->setEnabled(TRUE);
    window->hide();
    scrollArea->hide();

    emit closed();

}

/***
Function: cancel_window
***/
void priority::cancel_window() {

    this->setDisabled(TRUE);
    mom->setEnabled(TRUE);
    window->hide();
    scrollArea->hide();

    emit exit();

}





