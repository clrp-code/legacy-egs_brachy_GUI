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

#ifndef File_selector_HH
#define File_selector_HH

#define TRUE 1
#define FALSE 0
#include <QtWidgets>

class File_selector: public QWidget {
private:
    Q_OBJECT


    QString home_path;

    QString initial_egsphant;
    QString initial_egsinp;
    QString initial_transformation;
    QString initial_egsinp_header;


    //Data
    QLabel *egsphant_location;
    QLabel *egsinp_location;
    QLabel *transformation_location;


    QHBoxLayout *egsphant;
    QHBoxLayout *egsinp;
    QHBoxLayout *transf;

    QGroupBox *phantFrame;
    QGroupBox *transfFrame;
    QGroupBox *inpFrame;
    QGroupBox *headerFrame;

    QPushButton *select1;
    QPushButton *select2;
    QPushButton *select3;

    QGridLayout *File_selector_layout;

    QPushButton *apply;
    QPushButton *cancel;

    QPlainTextEdit *description;
    QLabel *egs_header;

    QString egsinp_default;
    QString egsphant_default;
    QString transf_default;

    void check_valid_egsinp(QString file, QString existing_valid);

public:
    //Constructor
    File_selector(QWidget *parent, QString EGSHOME, QString egsinput, QString egsphant, QString transf);
    //Decnstructor
    ~File_selector();

    QWidget *mom;
    QWidget *window;

    QString egsinp_header;

    QLabel *egsphant_path;
    QLabel *egsinp_path;
    QLabel *transformation_path;


public slots:
    void close_file_selector();
    void save_file_selector();
    void change_egsinp();
    void change_egsphant();
    void change_transformation();



public:
// LAYOUT FUNCTIONS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void createLayout();
    void connectLayout();
    void save_info_startup();

    QString initial_path;
    bool changed_location = false;



signals:
    void closed();
    void cancel_pressed();

};
#endif