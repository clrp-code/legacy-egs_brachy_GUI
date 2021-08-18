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
#include <fstream>
#include <sstream>
#include <string>

#define TRUE 1
#define FALSE 0


#ifndef metrics_h
#define metrics_h

struct DVHpoints {
    double dose;
    double vol;
};

struct metrics_data {
    QString name;
    QString plot_path; //output in folder with the path name as the media name
    QString plot_data;
    QVector <double> Dx, Vx; //V90, D100.., values
    double min, eMin, max, eMax, avg, eAvg, err, maxErr, totVol, nVox, precentVox;
    double HI, CI;

};



class metrics : public QWidget {
private:
    Q_OBJECT


    QGridLayout *layout;
    QPushButton *close;
    //QPushButton *customStats;

    QComboBox *mediaBox;
    QStringList *mediaItems;
    QLabel *mediaLabel;
    QHBoxLayout *mediaLayout;
    QGroupBox *headerBox;

    QLabel *metricsLabel;
    QGridLayout *metricsLayout;
    QGroupBox *metricsFrame;
    QGroupBox *mediaFrame;


    QLabel *D90Label;
    QLabel *D50Label;
    QLabel *D60Label;
    QLabel *D70Label;
    QLabel *D80Label;

    QLabel *Dx0;
    QLabel *Dx1;
    QLabel *Dx2;
    QLabel *Dx3;
    QLabel *Dx4;

    QHBoxLayout *D90Layout;
    QHBoxLayout *D80Layout;
    QHBoxLayout *D70Layout;
    QHBoxLayout *D60Layout;
    QHBoxLayout *D50Layout;
    QPushButton *viewDVH;


    QLabel *V90Label;
    QLabel *V100Label;
    QLabel *V150Label;
    QLabel *V200Label;

    QLabel *Vx0;
    QLabel *Vx1;
    QLabel *Vx2;
    QLabel *Vx3;

    QHBoxLayout *V90Layout;
    QHBoxLayout *V100Layout;
    QHBoxLayout *V150Layout;
    QHBoxLayout *V200Layout;

    QLabel *volLabel;
    QGridLayout *volLayout;
    QGroupBox *volFrame;

    QHBoxLayout *VoxVolLayout;
    QLabel *VoxVolLabel;
    QLabel *VoxVol;

    QHBoxLayout *VoxNumLayout;
    QLabel *VoxNumLabel;
    QLabel *VoxNum;

    QLabel *meanLabel;
    QLabel *mean;
    QHBoxLayout *meanLayout;
    QLabel *peakLabel;
    QLabel *peak;
    QHBoxLayout *peakLayout;
    QLabel *minLabel;
    QLabel *min;
    QHBoxLayout *minLayout;
    QLabel *HILabel;
    QLabel *HI;
    QHBoxLayout *HILayout;
    QLabel *CILabel;
    QLabel *CI;
    QHBoxLayout *CILayout;

    QGridLayout *statsLayout;
    QGroupBox *statsFrame;

    QPushButton *mediaStats;
    QPushButton *allStats;
    QGridLayout *outputLayout;
    QGroupBox *outputFrame;


// PROGRESS BAR~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    double *remainder;
    QWidget *progWin;
    QGridLayout *progLayout;
    QProgressBar *progress;
    QLabel *progLabel;
    //QPushButton *cancel2;


public:

    QWidget *window;



    void get_data(int nx, int ny, int nz, QVector<double> xbounds, QVector<double> ybounds, QVector<double> zbounds, QVector <QVector <QVector <int> > > media,
                  QVector<double> val, QVector<double> err, QMap <int, QString> contour_tas_name);  //Initializes the metrics class



    // LAYOUT FUNCTIONS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void createLayout();
    void connectLayout();


public slots:
    void changeMetrics();   //Updates the dose metrics when the contour is changed
    void outputStats();     //Outputs the metrics for the selected contour
    void outputStatsAll();  //Outputs the metrics for all contours
    void showGrace();       //Shows the xmgrace plot in a new window through the console
    void closeWindow();     //closes the metrics window


private:

    //values obtained from the 3ddose file
    int x, y, z;
    QVector<double> xbound, ybound, zbound;
    QVector <QVector <QVector <int> > > media_vect;
    QMap <int, QString> unique_media;
    QVector<double> val_vect;
    QVector<double> err_vect;

    bool error_empty = false;

    QVector<metrics_data> metric_data;      //metrics for all the contours

    QVector<double> dvhVol = {90, 100, 150, 200};       //dose metrics to calculate
    QVector<double> dvhDose = {50, 60, 70, 80, 90};     //volume metrics to calculate
    int dNum = 5; //dvhDose.size();                     //size of dose metrics
    int vNum = 4; //dvhVol.size();                      //size of volume metrics

    void plotDVH();                                      //Creates the values for the DVH plots
    void merge(DVHpoints *data, int n);                  //sorting alogarithim
    void submerge(DVHpoints *data, int i, int c, int f); //sorting algorithim
    int get_idx_from_ijk(int ii, int jj, int kk);        //used to manipulate indicies in the phantom

    // Return a QString containing DVH for xmgrace for a contour
    QString plot_region(int med,
                        double *min, double *eMin, double *max, double *eMax,
                        double *avg, double *eAvg, double *meanErr, double *maxErr,
                        double *totVol, double *nVox, QVector <double> *Dx,
                        QVector <double> *Vx);

    // Return a QString containing DVH for xmgrace for the entire phantom
    QString plot(double *min, double *eMin, double *max, double *eMax,
                 double *avg, double *eAvg, double *meanErr, double *maxErr,
                 double *totVol, double *nVox, QVector <double> *Dx,
                 QVector <double> *Vx);

    QString plot_noerror(double *min, double *eMin, double *max, double *eMax,
                         double *avg, double *eAvg, double *meanErr,
                         double *totVol, double *nVox, QVector <double> *Dx,
                         QVector <double> *Vx);

    double doseSearch(DVHpoints *data, int i, int c, int f, double *dose);  //Used to calculate the dose metrics
    double volSearch(DVHpoints *data, int i, int c, int f, double *vol);        //Used to calculate the volume metrics


    void doneGrace();                   // Close xmgrace
    void Grace(QString path);           // Call xmgrace to display the plot at path

    double small_increment;             //Used to update the progress bar

    void updateProgress(double n);
    void setup_progress_bar(QString window_title, QString text);



signals:
    void closed();


};







#endif

