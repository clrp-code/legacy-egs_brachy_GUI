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

#include "metrics.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

/*

    Creates a window for the 3ddose metrics and creates DVH plots

    Code to create the DVH plot and calculate metrics was obtained from Martin Martinov's 3ddose tools

*/




//Dose delivered to all voxels contained in a specific region or organ of interest
//Marc chamberland python bit bucket has additional metrics

/***
Metrics:
Organ and D_value/length
          V_value
Organ volume > Value Gy
Peak dose for organ

Use the egsphant tissue medium and dose voxel vectors

DVH: frequency of distribution of dose within a defined volume (ptv, whole body..)
     displayed as %volume of total volume
        Direct (differential) DVH: sum voxels within a given range and plots the volume as a function of dose
        Cumulative (integral) DVH: more popular -calc the volume of targe that recieve at least the given dose and plots this against dose
                                                -starts at 100% of volume (0 Gy)

Inplement from martin
    -Creates historgram (min, max target dose)
    -Mean target dose
    -Comparison parameters -historgram & output file with data (volume, numb voxels, RMS..)
    -xm grace plot dose volume historgram
        -DVH, DVHmed, DVHreg

https://www.ncbi.nlm.nih.gov/pmc/articles/PMC2253950/
***/


/***
Function: get_data
-------------------
Process: Initializes the metrics class

Inputs: nx, ny, nz: number of voxels in the x,y,z plane
        xbounds, ybounds, zbounds: the voxel boundaries in the x, y, z planes
        media: phantom of DICOM contours
        val: dose value array from the 3ddose file
        err: error value array from the 3ddose file
        contour_tas_name: vector of contour names, order in the vector correlates with it's index in the media array
***/
void metrics::get_data(int nx, int ny, int nz, QVector<double> xbounds, QVector<double> ybounds, QVector<double> zbounds,
                       QVector <QVector <QVector <int> > > media, QVector<double> val, QVector<double> err, QMap <int, QString> contour_tas_name) {

    QTextStream out(stdout);
    out<<endl <<"Calculating metrics..." <<endl;

    x = nx;
    y = ny;
    z = nz;
    xbound = xbounds;
    ybound = ybounds;
    zbound = zbounds;
    media_vect = media;
    unique_media = contour_tas_name;
    val_vect = val;
    err_vect = err;

    // Progress Bar ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    remainder = new double (0.0);
    progWin = new QWidget();
    progLayout = new QGridLayout();
    progress = new QProgressBar();
    progLabel = new QLabel();
    progLabel->setFont(QFont("Serif", 12, QFont::Normal, false));
    progLayout->addWidget(progress, 0, 0);
    progWin->setLayout(progLayout);
    progWin->resize(300, 0);
    progress->setRange(0, 1000000000);
    progWin->setFont(QFont("Serif", 12, QFont::Normal, false));

    plotDVH();
    out<<"Successfully calculated metrics." <<endl;
    createLayout();
    connectLayout();

}


/***
Function: plotDVH
-----------------
Process: this function creates the xmgrace plot and calculates the metrics for each
            contour individually and the entire phantom

Coppied from Martin Martinov's 3ddose tools
***/
void metrics::plotDVH() {
    QTextStream out(stdout);


    double increment = 1000000000;
    small_increment = increment*0.94;
    small_increment = small_increment/((z+6)*(unique_media.size()+1));
    updateProgress(increment*0.005);

    //User selects location and file name of dicom dose file
    //------------------------------------
    setup_progress_bar("Calculating metrics", "");
    updateProgress(increment*0.005);


    metric_data.reserve(unique_media.size() + 1);

    updateProgress(increment*0.005);

    double min, eMin, max, eMax, avg, eAvg, err, maxErr, totVol, nVox;

    QVector <double> Dx, Vx;

    //Create DVH for all media together
    QString path = "DVH_all_media.agr";
    QString plot_data;


    updateProgress(increment*0.005);

    int n = 0;
    QString name = "DVH Plot for All Media";
    plot_data = "@g0 type xy \n";
    plot_data += "@    title \"" + name + "\" \n";
    plot_data += "@    subtitle \"\" \n";
    plot_data += "@    legend on \n";
    plot_data += "@    legend box linestyle 0 \n";
    plot_data += "@    legend x1 0.6 \n";
    plot_data += "@    legend y1 0.75 \n";
    plot_data += "@    view xmin 0.250000 \n";
    plot_data += "@    xaxis  label \"Dose (Gy)\" \n";
    plot_data += "@    timestamp on \n";
    plot_data += "@    yaxis  label \"Volume (%)\" \n";
    plot_data += "@    s" + QString::number(n) + " on" + "\n";
    plot_data += "@    legend string  " + QString::number(n) + " \""
                 + "All Media"  + "\"" + "\n";
    plot_data += "@TYPE xy \n";
    plot_data += "@    s" + QString::number(n)
                 + " errorbar length 0.000000" + "\n";
    plot_data += "@    s" + QString::number(n)
                 + " symbol color " + QString::number(n+1) + "\n";

    updateProgress(increment*0.005);

    Dx = dvhDose;
    Vx = dvhVol;

    if (err_vect.size() == 0) {
        error_empty = true;
        plot_data += plot_noerror(&min, &eMin, &max, &eMax, &avg,
                                  &eAvg, &err, &totVol,
                                  &nVox, &Dx, &Vx);
    }
    else {
        plot_data += plot(&min, &eMin, &max, &eMax, &avg,
                          &eAvg, &err, &maxErr, &totVol,
                          &nVox, &Dx, &Vx);
    }


    plot_data +=  "\n";


    updateProgress(increment*0.005);


    updateProgress(increment*0.005);

    out<<"All contours" <<endl;

    updateProgress(increment*0.005);

    //Adding the data to the mretirs data structure
    metrics_data temp;

    temp.name = "All media";
    temp.plot_path = path;
    temp.plot_data = plot_data;
    temp.Dx = Dx;
    temp.Vx = Vx;
    temp.min = min;
    temp.eMin = eMin;
    temp.max = max;
    temp.eMax = eMax;
    temp.avg = avg;
    temp.eAvg = eAvg;
    temp.err = err;
    temp.maxErr = maxErr;
    temp.totVol = totVol;
    temp.nVox = nVox;
    int total_num_voxels = nVox;
    temp.HI = 1- (Vx[2]/Vx[1]); //1-(V_150/V_100)

    metric_data.push_back(temp);

    updateProgress(increment*0.005);

    QMapIterator<int, QString> map(unique_media);
    //Create DVH for each media
    while (map.hasNext()) {
        map.next();

        out<<map.value() <<endl;
        updateProgress(increment*0.005);

        QString path = "DVH_" + map.value() + ".agr";
        QString plot_data;

        QString name = "DVH Plot for " + map.value();
        plot_data = "@g0 type xy \n";
        plot_data += "@    title \"" + name + "\"\n";
        plot_data += "@    subtitle \"\"\n";
        plot_data += "@    legend on \n";
        plot_data += "@    legend box linestyle 0 \n";
        plot_data += "@    legend x1 0.6 \n";
        plot_data += "@    legend y1 0.75 \n";
        plot_data += "@    view xmin 0.250000 \n";
        plot_data += "@    xaxis  label \"Dose (Gy)\" \n";
        plot_data += "@    timestamp on \n";
        plot_data += "@    yaxis  label \"Volume (%)\" \n";

        int n = 0;

        plot_data += "@    s" + QString::number(n) + " on \n";
        plot_data += "@    legend string  " + QString::number(n) + " \""
                     + map.value()  + "\"" + "\n";
        plot_data += "@TYPE xy \n";
        plot_data += "@    s" + QString::number(n)
                     + " errorbar length 0.000000" + "\n";
        plot_data += "@    s" + QString::number(n)
                     + " symbol color " + QString::number(n+1) + "\n";

        updateProgress(increment*0.005);

        Dx = dvhDose;
        Vx = dvhVol;
        plot_data += plot_region(map.key(),
                                 &min, &eMin, &max, &eMax, &avg,
                                 &eAvg, &err, &maxErr, &totVol,
                                 &nVox, &Dx, &Vx);


        plot_data += "\n";


        metrics_data temp;

        updateProgress(increment*0.005);

        temp.name = map.value();
        temp.plot_path = path;
        temp.plot_data = plot_data;
        temp.Dx = Dx;
        temp.Vx = Vx;
        temp.min = min;
        temp.eMin = eMin;
        temp.max = max;
        temp.eMax = eMax;
        temp.avg = avg;
        temp.eAvg = eAvg;
        temp.err = err;
        temp.maxErr = maxErr;
        temp.totVol = totVol;
        temp.nVox = nVox;
        temp.precentVox = (nVox/total_num_voxels)*100;
        temp.HI = 1- (Vx[2]/Vx[1]); //1-(V_150/V_100)
        if (!metric_data.isEmpty()) { //the CI cannot be calculated for the first instance (the eniter body)
            temp.CI = Vx[1]/((1-Vx[1]) + metric_data[0].Vx[1]);    //V_100/((1-V_100)+ V_100(body))
        }
        metric_data.push_back(temp);

        updateProgress(increment*0.005);
    }

    progress->setValue(1000000000);

    progWin->hide();
    progWin->lower();


}


/***
Function: createLayout
***/
void metrics::createLayout() {
    QString style;
    style =  "QWidget {background-color: rgb(240, 240, 240)}";
    style += "QTextEdit";
    style += "{";
    style += "background-color: rgb(250, 250, 255);";
    style += "}";
    style += "QLineEdit";
    style += "{";
    style += "background: rgb(250, 250, 255)";
    style += "}";
    style += "QListWidget";
    style += "{";
    style += "background-color: rgb(250, 250, 255);";
    style += "}";
    style += "QToolTip {";
    style += "background-color: rgb(240, 240, 240);";
    style += "color: rgb(0, 0, 0);";
    style += "}";
    window = new QWidget();
    layout = new QGridLayout();
    close = new QPushButton(tr("Close"));

    //Creating title/header
    //---------------------------------
    //Header -user can select the medium name
    mediaItems = new QStringList();
    *mediaItems << "All media";
    QMapIterator<int, QString> map(unique_media);
    //Create DVH for each media
    while (map.hasNext()) {
        map.next();
        *mediaItems<<map.value();
    }

    mediaBox = new QComboBox();
    mediaBox->addItems(*mediaItems);
    mediaBox->setCurrentIndex(0);   //Displays the "All media" option

    mediaLayout = new QHBoxLayout();
    mediaLayout->addWidget(mediaLabel = new QLabel(tr("Contour Selected:")));
    mediaLayout->addWidget(mediaBox);
    headerBox = new QGroupBox();
    headerBox->setLayout(mediaLayout);
    headerBox->setPalette(QPalette(Qt::darkBlue));

    //Dose metrics
    //---------------------------------
    metricsLabel = new QLabel(tr("Dose Metrics  / Gy"));

    D50Layout = new QHBoxLayout();
    D50Layout->addWidget(D50Label = new QLabel("D<sub>50 </sub>: "));
    D50Layout->addWidget(Dx0 = new QLabel(QString::number(metric_data[0].Dx[0])));

    D60Layout = new QHBoxLayout();
    D60Layout->addWidget(D60Label = new QLabel("D<sub>60 </sub>: "));
    D60Layout->addWidget(Dx1 = new QLabel(QString::number(metric_data[0].Dx[1])));

    D70Layout = new QHBoxLayout();
    D70Layout->addWidget(D70Label = new QLabel("D<sub>70 </sub>: "));
    D70Layout->addWidget(Dx2 = new QLabel(QString::number(metric_data[0].Dx[2])));

    D80Layout = new QHBoxLayout();
    D80Layout->addWidget(D80Label = new QLabel("D<sub>80 </sub>: "));
    D80Layout->addWidget(Dx3 = new QLabel(QString::number(metric_data[0].Dx[3])));

    D90Layout = new QHBoxLayout();
    D90Layout->addWidget(D90Label = new QLabel("D<sub>90 </sub>: "));
    D90Layout->addWidget(Dx4 = new QLabel(QString::number(metric_data[0].Dx[4])));


    metricsLayout = new QGridLayout();
    metricsLayout->addWidget(metricsLabel, 0, 0);
    metricsLayout->addLayout(D50Layout, 1, 0);
    metricsLayout->addLayout(D60Layout, 2, 0);
    metricsLayout->addLayout(D70Layout, 3, 0);
    metricsLayout->addLayout(D80Layout, 4, 0);
    metricsLayout->addLayout(D90Layout, 5, 0);
    metricsFrame = new QGroupBox();
    metricsFrame->setLayout(metricsLayout);


    //Volume metrics
    //---------------------------------
    volLabel = new QLabel(tr("Volume Metrics  / %"));

    V90Layout = new QHBoxLayout();
    V90Layout->addWidget(V90Label = new QLabel("V<sub>90 </sub>: "));
    V90Layout->addWidget(Vx0 = new QLabel(QString::number(metric_data[0].Vx[0])));

    V100Layout = new QHBoxLayout();
    V100Layout->addWidget(V100Label = new QLabel("V<sub>100 </sub>: "));
    V100Layout->addWidget(Vx1 = new QLabel(QString::number(metric_data[0].Vx[1])));

    V150Layout = new QHBoxLayout();
    V150Layout->addWidget(V150Label = new QLabel("V<sub>150 </sub>: "));
    V150Layout->addWidget(Vx2 = new QLabel(QString::number(metric_data[0].Vx[2])));

    V200Layout = new QHBoxLayout();
    V200Layout->addWidget(V200Label = new QLabel("V<sub>200 </sub>: "));
    V200Layout->addWidget(Vx3 = new QLabel(QString::number(metric_data[0].Vx[3])));


    volLayout = new QGridLayout();
    volLayout->addWidget(volLabel, 0, 0);
    volLayout->addLayout(V90Layout, 1, 0);
    volLayout->addLayout(V100Layout, 2, 0);
    volLayout->addLayout(V150Layout, 3, 0);
    volLayout->addLayout(V200Layout, 4, 0);
    volFrame = new QGroupBox();
    volFrame->setLayout(volLayout);

    //Additional metrics
    //---------------------------------
    meanLabel = new QLabel(tr("Mean Dose [Gy]: "));
    peakLabel = new QLabel(tr("Peak Dose [Gy]: "));
    minLabel= new QLabel(tr("Minimum Dose [Gy]: "));

    if (error_empty) {
        mean = new QLabel(QString::number(metric_data[0].avg));
        peak = new QLabel(QString::number(metric_data[0].max));
        min = new QLabel(QString::number(metric_data[0].min));
    }
    else {
        mean = new QLabel(QString::number(metric_data[0].avg) + " ± " + QString::number(metric_data[0].eAvg));
        peak = new QLabel(QString::number(metric_data[0].max) + " ± " + QString::number(metric_data[0].eMax));
        min = new QLabel(QString::number(metric_data[0].min) + " ± " + QString::number(metric_data[0].eMin));

    }


    HILabel = new QLabel(tr("Homogeneity Index: "));
    HI = new QLabel(QString::number(metric_data[0].HI));

    CILabel = new QLabel("");   //the CI index is empty as can't be calculated for 'All media'
    CI = new QLabel("");

    VoxVolLabel = new QLabel(tr("Total Volume [cm<sup>3  </sup>]: "));
    VoxVol = new QLabel(QString::number(metric_data[0].totVol));

    VoxNumLabel = new QLabel(tr("Number of Voxels: "));
    VoxNum = new QLabel(QString::number(metric_data[0].nVox, 'g', 8));

    statsLayout = new QGridLayout();
    statsLayout->addWidget(meanLabel,0,0);
    statsLayout->addWidget(mean,0,1);
    statsLayout->addWidget(peakLabel,1,0);
    statsLayout->addWidget(peak,1,1);
    statsLayout->addWidget(minLabel,2,0);
    statsLayout->addWidget(min,2,1);
    statsLayout->addWidget(HILabel,3,0);
    statsLayout->addWidget(HI,3,1);
    statsLayout->addWidget(CILabel,4,0);
    statsLayout->addWidget(CI,4,1);
    statsLayout->addWidget(VoxVolLabel,5,0);
    statsLayout->addWidget(VoxVol,5,1);
    statsLayout->addWidget(VoxNumLabel,6,0);
    statsLayout->addWidget(VoxNum,6,1);

    statsFrame = new QGroupBox();
    statsFrame->setLayout(statsLayout);


    //Output metrics as text file
    //---------------------------------
    mediaStats = new QPushButton(tr("Output metrics for selected contour"));
    allStats = new QPushButton(tr("Output metrics for all contours"));


    //Create custom dose data
    //---------------------------------
    //customStats = new QPushButton(tr("Calculate custom metrics"));


    outputLayout = new QGridLayout();
    //outputLayout->addWidget(customStats,0,0);
    outputLayout->addWidget(mediaStats,1,0);
    outputLayout->addWidget(allStats,2,0);
    outputLayout->addWidget(close,2,1);
    outputFrame = new QGroupBox();
    outputFrame->setLayout(outputLayout);


    layout->setSpacing(0);

    layout->addWidget(headerBox, 0,0,1,2);
    layout->addWidget(metricsFrame, 1,0);
    layout->addWidget(volFrame,1,1);
    layout->addWidget(statsFrame, 3,0,1,2);
    layout->addWidget(viewDVH = new QPushButton("View DVH Plot"), 4,0,1,2);
    layout->addWidget(outputFrame,5,0,2,2);

    window->setStyleSheet(style);
    window->setLayout(layout);
    window->setHidden(TRUE);
    window->setWindowTitle(tr("Metrics"));


}



/***
Function: connectLayout
***/
void metrics::connectLayout() {
    connect(mediaBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeMetrics()));
    connect(mediaStats, SIGNAL(clicked()),
            this, SLOT(outputStats()));
    connect(allStats, SIGNAL(clicked()),
            this, SLOT(outputStatsAll()));
    connect(viewDVH, SIGNAL(clicked()),
            this, SLOT(showGrace()));

    connect(close, SIGNAL(clicked()),
            this, SLOT(closeWindow()));


}







/***
Function: setup_progress_bar
-----------------------------
Process: Creates the progress bar
Inputs: window_title: the progress bar window title
        text: the text label of the progress bar
***/
void metrics::setup_progress_bar(QString window_title, QString text) {
    progress->reset();
    *remainder = 0;
    this->setDisabled(true);
    progWin->setWindowTitle(window_title);
    if (!text.isEmpty()) {
        progLabel->setText(text);
    }
    progWin->show();
    progWin->activateWindow();
    progWin->raise();

}

/***
Function: updateProgress (from martin's interface.cpp 3ddose tools)
-------------------------
Process: Used to update the progress bar

Code Obtained from Marton Martinov's 3ddose tools
***/
void metrics::updateProgress(double n) {
    // The flooring function rounds down a real number to the nearest integer
    // In this line, we remove the remainder from the total number
    *remainder += n - floor(n);

    progress->setValue(int(progress->value() + floor(n) + floor(*remainder))); //Incremeent
    progress->update(); // We redraw the bar
    QApplication::processEvents();

    // And if our remainder makes a whole number, remove it
    *remainder -= floor(*remainder);
}


/***
Function: showGrace
-------------------
Outputs the DVH plot to file location and shows the plot
***/
void metrics::showGrace() {
    QTextStream out(stdout);
    out<<"Displaying plot in pop-up." <<endl;


    QString DVH_plot = QFileDialog::getSaveFileName(0, "Save the DVH Plot",
                       metric_data[mediaBox->currentIndex()].plot_path, "agr (*.agr)");

    if (!DVH_plot.isEmpty()) {
        metric_data[mediaBox->currentIndex()].plot_path = DVH_plot;
        QFile *file;
        QTextStream *input;
        file = new QFile(DVH_plot);

        if (file->open(QIODevice::WriteOnly | QIODevice::Text)) { //Creating the code for the image
            input = new QTextStream(file);
            *input << metric_data[mediaBox->currentIndex()].plot_data;
        }


        Grace(metric_data[mediaBox->currentIndex()].plot_path);
    }

}


/**
Function: changeMetrics
-----------------------
Process: This function is used to udate the window's values when the mediaBox (contour) has been changed

***/
void metrics::changeMetrics() {
    int index = mediaBox->currentIndex();

    Dx0->setText(QString::number(metric_data[index].Dx[0]));
    Dx1->setText(QString::number(metric_data[index].Dx[1]));
    Dx2->setText(QString::number(metric_data[index].Dx[2]));
    Dx3->setText(QString::number(metric_data[index].Dx[3]));
    Dx4->setText(QString::number(metric_data[index].Dx[4]));

    Vx0->setText(QString::number(metric_data[index].Vx[0]));
    Vx1->setText(QString::number(metric_data[index].Vx[1]));
    Vx2->setText(QString::number(metric_data[index].Vx[2]));
    Vx3->setText(QString::number(metric_data[index].Vx[3]));


    mean->setText(QString::number(metric_data[index].avg) + " ± " + QString::number(metric_data[index].eAvg));
    peak->setText(QString::number(metric_data[index].max) + " ± " + QString::number(metric_data[index].eMax));
    min->setText(QString::number(metric_data[index].min) + " ± " + QString::number(metric_data[index].eMin));
    HI->setText(QString::number(metric_data[index].HI));

    //VoxNum->setText(QString::number(metric_data[index].nVox, 'g', 8));
    VoxVol->setText(QString::number(metric_data[index].totVol));

    if (index != 0) {
        CILabel->setText("Conformity Index: ");
        CI->setText(QString::number(metric_data[index].CI));
        VoxNum->setText(QString::number(metric_data[index].nVox, 'g', 8) + " (" + QString::number(metric_data[index].precentVox, 'f', 2) + "%)");
        //CILayout->setToolTips("Measure of how conformal the delivered \n
        //                      dose is to the target");
    }
    else {
        CILabel->setText("");
        CI->setText("");
        VoxNum->setText(QString::number(metric_data[index].nVox, 'g', 8));
    }

}


/***
Function: outputStats
---------------------
Outputs the metrics to a .txt file  for the contour that is selected by the media box
***/
void metrics::outputStats() {
    QTextStream out(stdout);


    int index = mediaBox->currentIndex();


    QStringList extra;
    extra << "Media            |"
          << "                 |"
          << "Minimum Dose     |"
          << "Maximum Dose     |"
          << "Average Dose     |"
          << "Maximum Error    |"
          << "Average Error    |"
          << "Total Volume     |"
          << "Number of Voxels |";

    for (int i = 0; i < dNum; i++) {
        extra << QString("D") + QString::number(dvhDose[i]).leftJustified(16) + "|";
    }
    for (int i = 0; i < vNum; i++) {
        extra << QString("V") + QString::number(dvhVol[i]).leftJustified(16) + "|";
    }

    // Output data text file
    extra[0] +=  metric_data[index].name;
    extra[1] += "Value          Error          |";

    extra[2] += QString::number(metric_data[index].min).leftJustified(14) + " " +
                QString::number(metric_data[index].eMin).leftJustified(14) + " |";
    extra[3] += QString::number(metric_data[index].max).leftJustified(14) + " " +
                QString::number(metric_data[index].eMax).leftJustified(14) + " |";
    extra[4] += QString::number(metric_data[index].avg).leftJustified(14) + " " +
                QString::number(metric_data[index].eAvg).leftJustified(14) + " |";
    extra[5] += QString::number(metric_data[index].maxErr).leftJustified(14) + " "
                + "               |";
    extra[6] += QString::number(metric_data[index].err).leftJustified(14) + " "
                + "               |";
    extra[7] += QString::number(metric_data[index].totVol).leftJustified(14) + " "
                + "               |";
    extra[8] += QString::number(metric_data[index].nVox).leftJustified(14) + " "
                + "               |";

    int count = 9;
    for (int j = 0; j < dNum; j++)
        extra[count++] +=
            QString::number(metric_data[index].Dx[j]).leftJustified(19) +
            "           |";

    for (int j = 0; j < vNum; j++)
        extra[count++] +=
            QString::number(metric_data[index].Vx[j]).leftJustified(19) +
            "           |";



    //Output the addiional statistics
    QString path2 = QFileDialog::getSaveFileName(0, tr("Save the Additional Statistics File "),
                    "/data/data060/sjarvis/Additional_Statistics/additional_statistics_" + metric_data[index].name + ".txt", tr("text (*.txt)"));

    //QString path2 = "/home/sjarvis/Qt/ct_gui/Additional_Statistics_" + metric_data[index].name + ".txt";
    QFile file2(path2);

    if (file2.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream input2(&file2);

        input2 << "Additional Statistics for DVH";
        input2 << "\n\n";

        for (int i = 0; i < extra.size(); i++) {
            input2 << extra[i] << "\n";
        }

        file2.close();
        out<<"Created output statistics for " <<metric_data[index].name <<endl;
    }

}


/***
Function: outputStatsAll
------------------------
Creates and outputs a text file with the metrics for all contours
***/
void metrics::outputStatsAll() {
    QTextStream out(stdout);

    QStringList output;
    output.reserve(metric_data.size()*(8+dNum+vNum));

    for (int index=0; index<metric_data.size(); index++) {

        QStringList extra;
        extra << "Media            |"
              << "                 |"
              << "Minimum Dose     |"
              << "Maximum Dose     |"
              << "Average Dose     |"
              << "Maximum Error    |"
              << "Average Error    |"
              << "Total Volume     |"
              << "Number of Voxels |";

        for (int i = 0; i < dNum; i++) {
            extra << QString("D") + QString::number(dvhDose[i]).leftJustified(16) + "|";
        }
        for (int i = 0; i < vNum; i++) {
            extra << QString("V") + QString::number(dvhVol[i]).leftJustified(16) + "|";
        }

        // Output metric_data text file
        extra[0] +=  metric_data[index].name;
        extra[1] += "Value          Error          |";

        extra[2] += QString::number(metric_data[index].min).leftJustified(14) + " " +
                    QString::number(metric_data[index].eMin).leftJustified(14) + " |";
        extra[3] += QString::number(metric_data[index].max).leftJustified(14) + " " +
                    QString::number(metric_data[index].eMax).leftJustified(14) + " |";
        extra[4] += QString::number(metric_data[index].avg).leftJustified(14) + " " +
                    QString::number(metric_data[index].eAvg).leftJustified(14) + " |";
        extra[5] += QString::number(metric_data[index].maxErr).leftJustified(14) + " "
                    + "               |";
        extra[6] += QString::number(metric_data[index].err).leftJustified(14) + " "
                    + "               |";
        extra[7] += QString::number(metric_data[index].totVol).leftJustified(14) + " "
                    + "               |";
        extra[8] += QString::number(metric_data[index].nVox).leftJustified(14) + " "
                    + "               |";
        int count = 9;
        for (int j = 0; j < dNum; j++)
            extra[count++] +=
                QString::number(metric_data[index].Dx[j]).leftJustified(19) +
                "           |";
        for (int j = 0; j < vNum; j++)
            extra[count++] +=
                QString::number(metric_data[index].Vx[j]).leftJustified(19) +
                "           |";

        output.append(extra);
        output.append("\n\n\n");
    }

    //Output the addiional statistics
    QString path2 = QFileDialog::getSaveFileName(0, tr("Save File"),
                    "/data/data060/sjarvis/Additional_Statistics/additional_statistics.txt", tr("text (*.txt)"));

    QFile file2(path2);




    if (file2.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream input2(&file2);

        input2 << "Additional Statistics for DVH";
        input2 << "\n\n";

        for (int i = 0; i < output.size(); i++) {
            input2 << output[i] << "\n";
        }

        file2.close();
        out<<"Created output statistics for all contours" <<endl;

    }


}



int metrics::get_idx_from_ijk(int ii, int jj, int kk)
// get_idx_from_ijk takes and (i,j,k) tuple of a 3ddose distribution
// and returns the voxel index
{
    return ii+jj*x+kk*x*y;
}






/***
Function: plot_region
----------------------
Creates the data for the xmgrace DVH plot for one contour

Inputs: med: the char of the contour in the medium_vect
        min: the minimum value of the dose value array
        eMin: the corresponding error for the minimum dose array value
        max: the maximum value of the dose value array
        eMax: the corresponding error for the maximum dose array value
        avg: the average dose array value
        eAvg: the error of the average dose array value
        meanErr: the mean error
        maxErr: the maximum error
        totVol: the total volume of the contour
        nVox: the total number of voxels in the contour
        Dx: the dose metric values
        Vx: the volume metric values

Code obtained from Martin Martinov's 3ddose tools
***/
QString metrics::plot_region(int med,
                             double *min, double *eMin, double *max, double *eMax, double *avg, double *eAvg, double *meanErr, double *maxErr,
                             double *totVol, double *nVox, QVector <double> *Dx, QVector <double> *Vx) {

    // Build a QString that holds a curve to be plotted in xmgrace
    QString output = "";
    updateProgress(small_increment);
    double total = 0;



    // Allocate enough data to hold every dose
    DVHpoints *data = new DVHpoints [x*y*z];
    int n = 0;
    double v = 0;

    // Aditional values defaults
    *eMin = *eMax = *eAvg = *meanErr = *avg = *totVol = 0;
    //int counter = 0;
    // Iterate through every voxel for either the whole thing, a region or
    // several media
    for (int k = 0; k < z; k++) {
        for (int j = 0; j < x; j++) {
            for (int i = 0; i < y; i++) {
                if (media_vect[i][j][k] == med) {

                    if (n == 0) { //initialize the values
                        *max = *min = val_vect[get_idx_from_ijk(i,j,k)];
                        *eMin = *eMax = err_vect[get_idx_from_ijk(i,j,k)];
                        *maxErr = err_vect[get_idx_from_ijk(i,j,k)];
                    }

                    // Compute averages first
                    v = (xbound[i+1]-xbound[i])*(ybound[j+1]-ybound[j])*(zbound[k+1]-zbound[k]);
                    *totVol += v;
                    *avg += val_vect[get_idx_from_ijk(i,j,k)]*v;
                    *eAvg += pow(err_vect[get_idx_from_ijk(i,j,k)],2)*v;
                    *meanErr += err_vect[get_idx_from_ijk(i,j,k)]*v;

                    // Check mins & maxs
                    if (*max < val_vect[get_idx_from_ijk(i,j,k)]) {
                        *max = val_vect[get_idx_from_ijk(i,j,k)];
                        *eMax = err_vect[get_idx_from_ijk(i,j,k)];
                    }
                    if (*min > val_vect[get_idx_from_ijk(i,j,k)]) {
                        *min = val_vect[get_idx_from_ijk(i,j,k)];
                        *eMin = err_vect[get_idx_from_ijk(i,j,k)];
                    }
                    if (*min == val_vect[get_idx_from_ijk(i,j,k)])
                        if (*eMin < err_vect[get_idx_from_ijk(i,j,k)]) {
                            *min = val_vect[get_idx_from_ijk(i,j,k)];
                            *eMin = err_vect[get_idx_from_ijk(i,j,k)];
                        }
                    if (*maxErr < err_vect[get_idx_from_ijk(i,j,k)]) {
                        *maxErr = err_vect[get_idx_from_ijk(i,j,k)];
                    }

                    // Add voxel dose to data[n]
                    data[n].dose = val_vect[get_idx_from_ijk(i,j,k)];
                    // Add voxel volume to data[n]
                    data[n].vol = v;
                    n++;

                }
            }
        }
        updateProgress(small_increment);
    }

    *avg /= (*totVol);
    *eAvg = sqrt(*eAvg/(*totVol));
    *meanErr /= (*totVol);
    *nVox = n;

    // This uses a mergesort algorithm to sort all the doses (and their appro-
    // priate volumes) from largest to smallest
    updateProgress(small_increment);

    merge(data, n);

    updateProgress(small_increment);

    for (int i = n-2; i >= 0; i--) {
        data[i].vol += data[i+1].vol;
    }
    total = data[0].vol; // Set total to the volume over which we histogram

    updateProgress(small_increment);

    // Here we compute Dx and Vx by using our newly sorted and cumulative data
    for (int i = 0; i < Vx->size(); i++) {
        (*Vx)[i] = doseSearch(data, 0, n/2, n, &((*Vx)[i]));
    }
    double tempVol;
    for (int i = 0; i < Dx->size(); i++) {
        tempVol = (*Dx)[i]/100.0*(*totVol);
        tempVol = (*Dx)[i]/100.0*(*totVol);
        (*Dx)[i] = volSearch(data, 0, n/2, n, &tempVol);
    }

    double increment = 1; // Set an increment to avoid creating an xmgrace file
    int index = 0;        // with more than 1000 data points
    if (n > 1000) {
        increment = double(n-1)/999.0;
    }

    updateProgress(small_increment);

    for (int i = 0; i < n && i < 1000; i++) {
        index = int(increment*i);
        output += "\t";
        output += QString::number(data[index].dose, 'E', 8);
        output += "\t";           // We take the fraction of the total and turn
        // it into a percentage
        output += QString::number(data[index].vol/total*100.0, 'E', 8);
        output += "\n";

    }

    delete[] data;
    updateProgress(small_increment);
    return output;
}


/***
Function: plot
----------------------
Creates the data for the xmgrace DVH plot for the entire phantom

Inputs: med: the char of the contour in the medium_vect
        min: the minimum value of the dose value array
        eMin: the corresponding error for the minimum dose array value
        max: the maximum value of the dose value array
        eMax: the corresponding error for the maximum dose array value
        avg: the average dose array value
        eAvg: the error of the average dose array value
        meanErr: the mean error
        maxErr: the maximum error
        totVol: the total volume of the contour
        nVox: the total number of voxels in the contour
        Dx: the dose metric values
        Vx: the volume metric values

Code obtained from Martin Martinov's 3ddose tools
***/
QString metrics::plot(double *min, double *eMin, double *max, double *eMax, double *avg, double *eAvg, double *meanErr, double *maxErr,
                      double *totVol, double *nVox, QVector <double> *Dx, QVector <double> *Vx) {

    // Build a QString that holds a curve to be plotted in xmgrace
    updateProgress(small_increment);

    QString output = "";

    double total = 0;

    *max = *min = val_vect[0];
    *maxErr = err_vect[0];

    // Allocate enough data to hold every dose
    DVHpoints *data = new DVHpoints [x*y*z];
    int n = 0;
    double v = 0;

    // Aditional values defaults
    *eMin = *eMax = *eAvg = *meanErr = *avg = *totVol = 0;

    // Iterate through every voxel for either the whole thing, a region or
    // several media


    for (int k = 0; k < z; k++) {
        for (int j = 0; j < y; j++) {
            for (int i = 0; i < x; i++) {

                // Compute averages first
                v = (xbound[i+1]-xbound[i])*(ybound[j+1]-ybound[j])*(zbound[k+1]-zbound[k]);
                *totVol += v;
                *avg += val_vect[get_idx_from_ijk(i,j,k)]*v;
                *eAvg += pow(err_vect[get_idx_from_ijk(i,j,k)]*val_vect[get_idx_from_ijk(i,j,k)],2)*v;
                *meanErr += err_vect[get_idx_from_ijk(i,j,k)]*val_vect[get_idx_from_ijk(i,j,k)]*v;

                // Check mins & maxs
                if (*max < val_vect[get_idx_from_ijk(i,j,k)]) {
                    *max = val_vect[get_idx_from_ijk(i,j,k)];
                    *eMax = err_vect[get_idx_from_ijk(i,j,k)]*val_vect[get_idx_from_ijk(i,j,k)];
                }
                if (*min > val_vect[get_idx_from_ijk(i,j,k)]) {
                    *min = val_vect[get_idx_from_ijk(i,j,k)];
                    *eMin = err_vect[get_idx_from_ijk(i,j,k)]*val_vect[get_idx_from_ijk(i,j,k)];
                }
                if (*min == val_vect[get_idx_from_ijk(i,j,k)])
                    if (*eMin < err_vect[get_idx_from_ijk(i,j,k)]) {
                        *min = val_vect[get_idx_from_ijk(i,j,k)];
                        *eMin = err_vect[get_idx_from_ijk(i,j,k)]*val_vect[get_idx_from_ijk(i,j,k)];
                    }
                if (*maxErr < err_vect[get_idx_from_ijk(i,j,k)]) {
                    *maxErr = err_vect[get_idx_from_ijk(i,j,k)]*val_vect[get_idx_from_ijk(i,j,k)];
                }

                // Add voxel dose to data[n]
                data[n].dose = val_vect[get_idx_from_ijk(i,j,k)];
                // Add voxel volume to data[n]
                data[n].vol = v;
                n++;

            }
        }
        updateProgress(small_increment);
    }

    *avg /= (*totVol);
    *eAvg = sqrt(*eAvg/(*totVol));
    *meanErr /= (*totVol);
    *nVox = n;

    updateProgress(small_increment);
    // This uses a mergesort algorithm to sort all the doses (and their appro-
    // priate volumes) from largest to smallest
    merge(data, n);

    updateProgress(small_increment);

    // Once all the DVHpoints in data have been sorted by dose, start at the
    // DVHpoint with the second smallest dose and add the volume of the smallest
    // dose DVHpoint.  Then for the third smallest dose of a DVHpoint add the
    // volume of the second smallest dose of a DVHpoint (which is the sum of
    // what was originally the smallest and second smallest volume) and continue
    // until at the very end you have the total volume at data[0]
    for (int i = n-2; i >= 0; i--) {
        data[i].vol += data[i+1].vol;
    }
    total = data[0].vol; // Set total to the volume over which we histogram

    updateProgress(small_increment);

    // Here we compute Dx and Vx by using our newly sorted and cumulative data
    for (int i = 0; i < Vx->size(); i++) {
        (*Vx)[i] = doseSearch(data, 0, n/2, n, &((*Vx)[i]));
    }
    double tempVol;
    for (int i = 0; i < Dx->size(); i++) {
        tempVol = (*Dx)[i]/100.0*(*totVol);
        (*Dx)[i] = volSearch(data, 0, n/2, n, &tempVol);
    }

    double increment = 1; // Set an increment to avoid creating an xmgrace file
    int index = 0;        // with more than 1000 data points
    if (n > 1000) {
        increment = double(n-1)/999.0;
    }

    updateProgress(small_increment);

    for (int i = 0; i < n && i < 1000; i++) {
        index = int(increment*i);
        output += "\t";
        output += QString::number(data[index].dose, 'E', 8);
        output += "\t";           // We take the fraction of the total and turn
        // it into a percentage
        output += QString::number(data[index].vol/total*100.0, 'E', 8);
        output += "\n";

    }


    delete[] data;
    updateProgress(small_increment);
    return output;

}



/***
Function: plot_noerror
----------------------
Creates the data for the xmgrace DVH plot for the entire phantom

Inputs: med: the char of the contour in the medium_vect
        min: the minimum value of the dose value array
        eMin: the corresponding error for the minimum dose array value
        max: the maximum value of the dose value array
        eMax: the corresponding error for the maximum dose array value
        avg: the average dose array value
        eAvg: the error of the average dose array value
        meanErr: the mean error
        maxErr: the maximum error
        totVol: the total volume of the contour
        nVox: the total number of voxels in the contour
        Dx: the dose metric values
        Vx: the volume metric values

Code obtained from Martin Martinov's 3ddose tools
***/
QString metrics::plot_noerror(double *min, double *eMin, double *max, double *eMax, double *avg, double *eAvg, double *meanErr,
                              double *totVol, double *nVox, QVector <double> *Dx, QVector <double> *Vx) {

    // Build a QString that holds a curve to be plotted in xmgrace
    updateProgress(small_increment);

    QString output = "";

    double total = 0;

    *max = *min = val_vect[0];                  //ERROR HERE


    // Allocate enough data to hold every dose
    DVHpoints *data = new DVHpoints [x*y*z];
    int n = 0;
    double v = 0;

    // Aditional values defaults
    *eMin = *eMax = *eAvg = *meanErr = *avg = *totVol = 0;

    // Iterate through every voxel for either the whole thing
    for (int k = 0; k < z; k++) {
        for (int j = 0; j < y; j++) {
            for (int i = 0; i < x; i++) {

                // Compute averages first
                v = (xbound[i+1]-xbound[i])*(ybound[j+1]-ybound[j])*(zbound[k+1]-zbound[k]);
                *totVol += v;
                *avg += val_vect[get_idx_from_ijk(i,j,k)]*v;

                // Check mins & maxs
                if (*max < val_vect[get_idx_from_ijk(i,j,k)]) {
                    *max = val_vect[get_idx_from_ijk(i,j,k)];
                }
                if (*min > val_vect[get_idx_from_ijk(i,j,k)]) {
                    *min = val_vect[get_idx_from_ijk(i,j,k)];

                }

                // Add voxel dose to data[n]
                data[n].dose = val_vect[get_idx_from_ijk(i,j,k)];
                // Add voxel volume to data[n]
                data[n].vol = v;
                n++;

            }
        }
        updateProgress(small_increment);
    }

    *avg /= (*totVol);
    *nVox = n;

    updateProgress(small_increment);
    // This uses a mergesort algorithm to sort all the doses (and their appro-
    // priate volumes) from largest to smallest
    merge(data, n);

    updateProgress(small_increment);

    // Once all the DVHpoints in data have been sorted by dose, start at the
    // DVHpoint with the second smallest dose and add the volume of the smallest
    // dose DVHpoint.  Then for the third smallest dose of a DVHpoint add the
    // volume of the second smallest dose of a DVHpoint (which is the sum of
    // what was originally the smallest and second smallest volume) and continue
    // until at the very end you have the total volume at data[0]
    for (int i = n-2; i >= 0; i--) {
        data[i].vol += data[i+1].vol;
    }
    total = data[0].vol; // Set total to the volume over which we histogram

    updateProgress(small_increment);

    // Here we compute Dx and Vx by using our newly sorted and cumulative data
    for (int i = 0; i < Vx->size(); i++) {
        (*Vx)[i] = doseSearch(data, 0, n/2, n, &((*Vx)[i]));
    }
    double tempVol;
    for (int i = 0; i < Dx->size(); i++) {
        tempVol = (*Dx)[i]/100.0*(*totVol);
        (*Dx)[i] = volSearch(data, 0, n/2, n, &tempVol);
    }

    double increment = 1; // Set an increment to avoid creating an xmgrace file
    int index = 0;        // with more than 1000 data points
    if (n > 1000) {
        increment = double(n-1)/999.0;
    }

    updateProgress(small_increment);

    for (int i = 0; i < n && i < 1000; i++) {
        index = int(increment*i);
        output += "\t";
        output += QString::number(data[index].dose, 'E', 8);
        output += "\t";           // We take the fraction of the total and turn
        // it into a percentage
        output += QString::number(data[index].vol/total*100.0, 'E', 8);
        output += "\n";

    }


    delete[] data;
    updateProgress(small_increment);
    return output;

}


/***
Function: merge
---------------
Inputs: data: the DVHpoints (dose and volume)
        n: the number of voxels
Obtained from Martin Martinov 3ddose tools
***/
void metrics::merge(DVHpoints *data, int n) {
    // The following sorting algorithm is referred to as a bottom-up (not
    // recursive) mergesort.  The idea of the algorithm is that adding two
    // sorted arrays of size n/2 into a sorted array of size n should only take
    // n iterations.
    // This is can be thought out pretty simply, we have two sorted arrays, l and
    // r, and a final array d.  l and r are of size n/2 and d is of size n.  We
    // can start by comparing l[0] and r[0].  The largest value of the two is
    // guaranteed to be the largest value of all n data points, so the larger
    // of l[0] and r[0] is added at d[0].  Then another comparison is made
    // between index 0 of the array that did not have the larger value and
    // index 1 of the array that did, and the larger of the two is set as d[1].
    // This process is repeated until d is filled (ie, if d[0] = l[0] then d[1]
    // will be the greater of l[1] and r[0]).
    // The above 'merging' algorithm can be applied to giant unsorted data sets
    // by first merging every 2 indices together (so n = 2 and n/2 = 1).  Once
    // every 2 indices are sorted, we can then go through and merge every 4
    // indices (so n = 4 and n/2 = 2), which works because we have already
    // the array by 2s.  So this is continued until all that's left of the array
    // are two separately sorted halves, which are then merged together in one
    // final iteration
    // This algorithm is O(n*log(n)), which makes sense because we sort all n
    // data points separately log(n) times.   This log is base 2.

    if (n <= 1) { // If our array is size 1 or less quit
        return;
    }

    int subn = 1, i = 0; // subn the size of subsections that are being
    // submerged and i is the current index of the array
    // at which we are at
    while (subn < n) { // While we are still submerging sections that are smaller
        // than the size of the array
        i = 0; // Reset the index to 0
        while (i < n - subn) { // And iterate through n/(2*subn) sections, truncated
            if (i + (2 * subn) < n) // submerge two subn sized portions of data
                // of the array
            {
                submerge(data, i, i + subn - 1, i + 2 * subn - 1);
            }
            else // Or submerge a subn sized section and whatever is left of the
                // array
            {
                submerge(data, i, i + subn - 1, n - 1);
            }
            i += 2 * subn; // Move the index two submerge the next 2 subsections
        }
        subn *= 2; // Double the size of subsection to be merged
    }

}


/***
Function: submerge
------------------
Function obtained from Martin Martinov 3ddose tools
***/
void metrics::submerge(DVHpoints *data, int i, int c, int f) {

    int l = i, r = c+1, j = 0; // We have three indices, l for one subsection,
    // r the other, and j for the new sorted array
    DVHpoints *temp = new DVHpoints [f-i+1]; // Set aside memory for sorted array
    while (l <= c && r <= f) { // While we have yet to iterate through either
        // subsection
        if (data[l].dose > data[r].dose) { // If value at r index is smaller then
            temp[j++] = data[r++];    // add it to temp and move to next r
        }
        else {                           // If value at l index is smaller then
            temp[j++] = data[l++];    // add it to temp and move to next l
        }
    }
    while (l <= c) { // Add all the remaining ls to temp (if any)
        temp[j++] = data[l++];
    }
    while (r <= f) { // Add all the remaining rs to temp (if any)
        temp[j++] = data[r++];
    }

    for (int k = 0; k < j; k++) {
        data[i+k] = temp[k];    // Reassign all the data values to the temp values
    }

    delete[] temp; // Delete temp

}


/***
Function: volSearch
-------------------
Function obtained from Martin Martinov 3ddose tools
***/
double metrics::volSearch(DVHpoints *data, int i, int c, int f, double *vol) {

    // This algorithm uses the fact that the data is sorted to eliminate half
    // of the possible data on which the point volume could lie, then when it
    // finds it, it returns the dose
    // This algorithm is O(log(n))

    if (c == i || (data[c].vol >= *vol && *vol > data[c+1].vol)) {
        return data[c].dose;
    }
    else if (c == f || (data[c-1].vol >= *vol && *vol > data[c].vol)) {
        return data[c-1].dose;
    }
    else if (data[c].vol > *vol) {
        return volSearch(data, c, (f+c)/2, f, vol);
    }
    else {
        return volSearch(data, i, (i+c)/2, c, vol);
    }

}


/***
Function: doseSearch
--------------------
Function obtained from Martin Martinov 3ddose tools
***/
double metrics::doseSearch(DVHpoints *data, int i, int c, int f, double *dose) {

    // This algorithm uses the fact that the data is sorted to eliminate half
    // of the possible data on which the point dose could lie, then when it
    // finds it, it returns the volume
    // This algorithm is O(log(n))

    if (c == i || (data[c].dose <= *dose && *dose < data[c+1].dose)) {
        return data[c].vol;
    }
    else if (c == f || (data[c-1].dose <= *dose && *dose < data[c].dose)) {
        return data[c-1].vol;
    }
    else if (data[c].dose < *dose) {
        return doseSearch(data, c, (f+c)/2, f, dose);
    }
    else {
        return doseSearch(data, i, (i+c)/2, c, dose);
    }

}


/***
Function: Grace
---------------
Function obtained from Martin Martinov 3ddose tools
***/
void metrics::Grace(QString path) {

    QProcess *grace = new QProcess();

    connect(grace, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(doneGrace()));

    grace->start(tr("xmgrace ") + path);

}


/***
Function: doneGrace
------------------
Closes the xmgrace process when user closes the plot

Function obtained from Martin Martinov 3ddose tools
***/
void metrics::doneGrace() {

    QProcess *grace = (QProcess *)sender();

    grace->terminate();
    delete grace;

}



/***
Function: closeWindow
------------------
Closes the metrics window
***/
void metrics::closeWindow() {
    this->setDisabled(TRUE);
    //mom->setEnabled(TRUE);
    window->hide();

    emit closed();

}















































