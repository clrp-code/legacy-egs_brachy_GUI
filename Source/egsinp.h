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

#include <QtWidgets>
#include <iostream>
#include <math.h>

#ifndef egsinp_h
#define egsinp_h

// struct coordinate;
struct coordinate {
    double x;
    double y;
    double z;
};

class egsinp: public QObject {
    Q_OBJECT


signals:
    void progressMade(double n); // Update the progress bar

public:

    void create_egsinp(QString element, QString seed_found, double DSF, QString treatment_type,
                       QVector <double> source_weights, QVector<QString> unique_tissue, bool tg43Flag,
                       bool checked_score_eneg_deposition, QString user_muen_file, double number_hist, double number_batch,
                       double number_chunk, QString header, QString transport_param_path, QString user_material_file);

    void create_egsinp_phantom(QString element, QString seed_found, QString phant_file,
                               QString transp_file, QString run_mode, bool checked_energy_depos,
                               QString selected_muen_file, double numb_histories, double numb_batch,
                               double numb_chunk, QString egsinp_header, QString material_data_file);




    QString egsinp_path;
    QString seed_file;
    QString egs_home;
    QString egsphant_location;
    QString egsinp_name;
    QString egs_brachy_home_path;

    void get_spectra_file_path(QString element);
    //void create_transformation_file(QVector<plan> seed_data, double air_kerma);
    void create_transformation_file(QVector<QVector<coordinate>> all_seed_pos, double air_kerma);

    bool read_spectra_file = false;
    // bool read_muen_file = false;
    // bool read_transport_parameter = false;

    //Varialbes obtained by reading from default_files.txt
    QVector<QString> spectra_files;
    QString spectra_file;
    QString muen_file_substring;
    QString low_energy_transport_file;
    QString high_energy_transport_file;
    QString material_data;

    QString boxFilePath;
    QString applicatorFilePath;

private:





};
#endif