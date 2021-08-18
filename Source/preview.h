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

#ifndef PREVIEW_H
#define PREVIEW_H

#define TRUE 1
#define FALSE 0
#include "egsphant.h"
//#include <QtWidgets>1,5


/*******************************************************************************
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Hover Label Class~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*******************************************************************************/
// This class is created to overwrite one of the QWidget functions to be able to
// keep track of the position of the mouse, and to know whether it is over the
// widget or not, so as to be able to give the coordinates of the seed

class HoverLabel : public QLabel { // It inherits QLabel publicly
private:
    Q_OBJECT // This line is necessary to create custom SLOTs, ie, functions
    // that define what happens when you click on buttons
public:
    void mouseMoveEvent(QMouseEvent *event); // Overwrite mouseMoveEvent to send
    // a signal to MainWindow
    void wheelEvent(QWheelEvent *event); // Overwrite wheelEvent to send
    // a signal to MainWindow
signals:
    void mouseMoved(int width, int height); // This is the signal to be
    // sent in mouseMoveEvent
    void mouseWheelUp();   // Detect the wheel being scrolled over the image
    void mouseWheelDown(); // to shift depth appropriately
};


class Preview: public QWidget {
private:
    Q_OBJECT



    //QVector<dicom_to_tas> contour;            //name of contour
    //QVector<int> unique_contour_mapping;  //index of unique contours

    QVector<QString> media_name; //unique media names
    QVector<QColor> med_colour_names;
    QVector<QColor>  contour_colour_names;

    QPixmap phantPicture;

    // Universal
    QGridLayout *layout;
    QPushButton *close;
    QPushButton *savePic;
    QLabel *pose;

    // Image Frame
    HoverLabel *image;
    QScrollArea *imageFrame;
    QPixmap *picture;

    // Image Dimensions
    QGroupBox *dimFrame;
    QStringList *dimItems;
    QComboBox *dimBox;
    QLabel *depth;
    QSlider *dimc;
    QToolButton *dimcLeft, *dimcRight;
    QGridLayout *dimLayout;

    // Image Resolution
    QLineEdit *resEdit;
    QHBoxLayout *resLayout;
    QGroupBox *resFrame;

    // Contour Selector
    QFrame *medFrame;
    QGridLayout *medLayout;
    QVector <QPushButton *> *med_colors;
    QVector <QLabel *> *media_names;

    // QVector <QPushButton*> * contour_colors1;
    // QVector <QLabel*> * contour_names1;
    // QGridLayout * contLayout1;
    // QVector<QColor>  cont_colour_names1;

    // //QVector <QPushButton*> *contour_colors;
    // QVector <QLabel*> *contour_name1;
    // QGridLayout *contourLayout1;
    // QFrame *contourFrame1;

    QLabel *redrawLabel;


    int get_idx_from_ijk(int ii, int jj, int kk);

    QImage getEGSPhantPicMed(QString axis, double ai, double af,
                             double bi, double bf, double d, int res);         //Creates the medium image
    int getMedia(double px, double py, double pz);                  //Returns the contour at coordinates


    double ai;      //stores the boundaries of the slice
    double af;
    double bi;
    double bf;

// PROGRESS BAR~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    double *remainder;
    QWidget *progWin;
    QGridLayout *progLayout;
    QProgressBar *progress;
    QLabel *progLabel;


public:
    //Constructor
    Preview(QWidget *parent, QMap <QString, unsigned char> mediaMap);
    //Decnstructor
    ~Preview();

    //Data
    EGSPhant phant;
    QMap <unsigned char, int> medCharMap;
    QVector <QVector <QPolygonF> > structPos;
    QVector <QVector <double> > structZ;
    QVector<bool> structUnique;
    QVector<QString> contourName;


    QErrorMessage *errors;
    QWidget *mom;
    QWidget *window;

    void createLayout();
    void connectLayout();
    void create_progress_bar();
    void createwindow();


public slots:
    void close_preview();
    void mouseGeom(int width, int height); // This updates pose when a
    // mouse signal is received
    void changeDim();                   // Change labeling on dimensions
    void changeDepthLeft();
    void changeDepthRight();
    void changeColor();                 // Change contour color
    //void changeColorContour();

    void updateImage();                 // This updates the image
    void redraw();                      //This creates the image
    void saveImage();                   //This allows the user to save the image
    void closePreview();                //This closes the previewer window


    void updateProgress(double n);          //Updates progress bar

signals:
    void closed();




};


#endif