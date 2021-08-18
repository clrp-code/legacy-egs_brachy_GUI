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

#ifndef TISSUE_CHECK_H
#define TISSUE_CHECK_H

#define TRUE 1
#define FALSE 0
#include <QtWidgets>

class Tissue_check: public QWidget {
private:
    Q_OBJECT

    QVector<QString> dicom_struct;      //ordered list of the dicom contours, unmodified name from DICOM
    QVector<int> predicted_organ;       //corresponding ordered list of the tas organs, unmodified name from TAS
    QVector<QString> organs_unique;             //list of all the organ names in the TAS

signals:
    void closed();
    void exit();

protected:
    void closeEvent(QCloseEvent *event) override;


public:
    //Constructor
    Tissue_check(QWidget *parent, QVector<QString> dicom_struct_name, QVector<QString> organ_names,QVector<QString> dicom_struct_type);
    //Decnstructor
    ~Tissue_check();

    void createLayout();
    void connectLayout();


//Tissue assingment scheme window------------------------------------------------
    QWidget *window;
    QGridLayout *tissLayout;
    QLabel *label;

    //Data
    QWidget *mom;

    bool generateImages = false;
    bool closed_check = false;
    QVector<bool> generateMask;

    QScrollArea *scrollArea;
    QPushButton *apply;
    QPushButton *cancel;
    QVector<QComboBox *> *organs;
    QVector<QLabel *> *dicom_cont;
    QVector< QCheckBox *> *mask_selection;
    QVector<QString> selected_tas_name;             //names of organs in tissue assignemnt scheme
    QCheckBox *image_selection ;

    QGridLayout *layout;


public slots:
    void close_tissue_check();
    void cancel_window();


};
#endif