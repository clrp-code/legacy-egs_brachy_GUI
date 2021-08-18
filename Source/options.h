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

#ifndef OPTIONS_H
#define OPTIONS_H

#define TRUE 1
#define FALSE 0
#include <QtWidgets>

class Options: public QWidget {
private:
    Q_OBJECT

    QString muen_on_startup;
    QString material_on_startup;
    int transp_index_startup = -1;

    double number_hist_startup;
    double number_chunk_startup;
    double number_batch_startup;
    bool checked_energy_startup;

    QString egsinp_path;

    QPushButton *cancel;
    QPushButton *apply;
    QPushButton *LoadButton;
    QPushButton *LoadMatButton;
    QRadioButton *score_yes;
    QLineEdit *numb_histories;
    QLineEdit *numb_batch;
    QLineEdit *numb_chunk;

    QGroupBox *transportBox;
    QPushButton *select_transport;
    QListWidget *transport_selection;

    QStringList transport_names;
    QStringList transport_names_path;
    QString muen_home;
    QString egs_brachy_home_path;

public:

    Options(QWidget *parent, QString egs_home);     //Constructor
    ~Options();                                     //Deconstructor

    //Data
    QWidget *mom;
    QWidget *window;


public slots:
    void close_options();
    void load_muen();
    void set_values();
    void load_material();


public:
// LAYOUT FUNCTIONS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void createLayout();
    void connectLayout();
    void save_options_on_startup(QString egsinp);
    void closeEvent(QCloseEvent *event);


    bool checked_score_energy_deposition = false;
    double number_hist = 1e8; //default value
    double number_batch = 1; //default value
    double number_chunk = 10; //default value
    QString muen_file;
    QString material_file;
    QString transport_param_path;

signals:
    void closed();


};
#endif