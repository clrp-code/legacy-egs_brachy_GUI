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

#include <QtGui>
#include <QtWidgets>

#include <fstream>
#include <iostream>
#include <iomanip>

#include <sstream>
#include <string>
#include <chrono>


#ifndef read_dose_h
#define read_dose_h

#define TRUE 1
#define FALSE 0

struct element {
    QString tag;
    int length;
    QString VR;
    QString value;
    QString transform;
};


class read_dose { //: public QObject

//signals:
//  void progressMade(double n); // Update the progress bar


public:

    int x, y, z;                 // The number of x, y and z voxels
    QVector <double> cx, cy, cz; // The actual x, y and z coordinates
    QVector <double> val;       // The values
    QVector <double>  err;      // The fractional errors
    bool flip = false;          //Flag identifies if the z-values need to be flipped (in decreasing order)
    char filled;                // Flag that says if the dose file is empty of not


    void create_dicom_dose(QMap<QString,QString> dicomHeaderData, QString path);
    void load_dose_data_comparison(QString path);
    void trim_dose(int xn, int yn, int zn, QString path_3ddose);

    void load_dose_data(QString path);

    QVector <QVector <QVector <double> > > valMatrix;
    QVector <QVector <QVector <double> > > errMatrix;


private:
    std::ofstream file_out;

    char *memblock_out;
    QVector<element> dictionary;


    QVector<int> pixel_data;
    double scaling;


    QVector<unsigned char> get_tag_output(QString tag);
    QVector<unsigned char> get_char_output(std::string str);
    QVector<unsigned char> get_char_output_copy(int length, std::string transform);
    QVector<unsigned char> get_length_output(std::string length);

    void output_pixel_data();
    void output_length_32(int length);
    void output_length_32_pixel(int length);
    void output_length_16(int length);

    double x_thick;
    double y_thick;
    double z_thick;

    double max_val;         //maximum dose value
    double min_val;         //minimum dose value

    double forward_mapping_factor;
    double reverse_mapping_factor;
    double offset;
    int min_float;

    void AddRemainingTags();
    void get_DoseGridScaling();
    void get_3ddose_data();

    QMap <QString, QString> dicomHeader;

    // PROGRESS BAR~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    double *remainder;
    QWidget *progWin;
    QGridLayout *progLayout;
    QProgressBar *progress;


    void create_progress_bar();
    void updateProgress(double n);
    void refresh();
};


#endif


