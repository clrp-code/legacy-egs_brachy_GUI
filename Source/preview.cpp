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

#include "preview.h"

/*

    This class creates a window to preview each slice of the phantom

    Code is obtained from Martin Martinov's 3ddose tools


*/
/*******************************************************************************
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Hover Label Class~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*******************************************************************************/
void HoverLabel::mouseMoveEvent(QMouseEvent *event) {
    if (event->x() > 0 && event->x() < width() && event->y() > 0 && event->y() < height()) {
        emit mouseMoved(event->x(),event->y());
    }
    event->accept();
}

void HoverLabel::wheelEvent(QWheelEvent *event) {
    if (event->delta() > 0 && event->x() > 0 && event->x() < width() && event->y() > 0 && event->y() < height()) {
        emit mouseWheelUp();
    }
    else if (event->x() > 0 && event->x() < width() && event->y() > 0 && event->y() < height()) {
        emit mouseWheelDown();
    }
    event->accept();
}

void Preview::createwindow() {
    QTextStream out(stdout);
    out<<"Creating Previewer" <<endl;

    //changeDim();

    createLayout(); // This creates the layout structure
    connectLayout(); // This connects the layout with the SLOTs
    window->resize(1200, 1050);

    //out<<"Created Previewer" <<endl;

    redraw();

}


Preview::Preview(QWidget *parent, QMap <QString, unsigned char> mediaMap)
    : QWidget(parent) { // Pass the parameter to the QWidget constructor

    mom = parent;
    medCharMap.clear();
    //Media map contaisn mapping from phant number to media
    //Can find char from media name - need to changed around
    QMapIterator<QString, unsigned char> i(mediaMap);
    while (i.hasNext()) {
        i.next();
        int j;

        //Convert the char in the media to an int
        if (i.value() >= 97) {
            j=i.value() - 49 - 7 - 6;
        }
        else if (i.value() >= 65) {
            j=i.value() - 49 - 7;
        }
        else {
            j= i.value() - 49;
        }
        medCharMap.insert(i.value(),j);
    }
}


Preview::~Preview() {
    //delete
    // for (int i = 0; i < med_colors->size(); i++)
    // delete (*med_colors)[i];
    // med_colors->clear();

    // delete layout;
    // delete close;
    // delete savePic;
    // delete pose;
    // delete image;
    // delete imageFrame;
    // delete picture;


    // delete dimItems;
    // delete dimBox;
    // delete depth;
    // delete dimc;
    // delete dimcLeft;
    // delete dimcRight;
    // delete dimLayout;
    // delete dimFrame;

    // delete resEdit;
    // delete resLayout;
    // delete resFrame;

    // delete medLayout;
    // delete medFrame;

    // delete med_colors;
    // delete media_names;

}



void Preview::createLayout() {

    errors = new QErrorMessage();
    errors->setHidden(TRUE);
    errors->setWindowTitle(tr("Error"));

    window = new QWidget();
    layout = new QGridLayout();
    close = new QPushButton("Close");
    savePic = new QPushButton("Save Image");

    //Creating the image
    picture = new QPixmap();
    image = new HoverLabel();
    image->setMouseTracking(true);
    image->setAlignment(Qt::AlignCenter);
    image->setPixmap(*picture);
    imageFrame = new QScrollArea();
    imageFrame->setFrameStyle(QFrame::Plain);
    imageFrame->setLineWidth(0);
    imageFrame->setWidget(image);

    //Creating the first column
    dimItems = new QStringList();
    *dimItems << "z axis" << "y axis" << "x axis";
    dimBox = new QComboBox();
    dimBox->addItems(*dimItems);

    depth = new QLabel("z Depth");
    dimc  = new QSlider(Qt::Horizontal);
    dimc->setSingleStep(1);
    dimcLeft = new QToolButton();
    dimcLeft->setArrowType(Qt::LeftArrow);
    dimcRight = new QToolButton();
    dimcRight->setArrowType(Qt::RightArrow);
    dimLayout = new QGridLayout();
    dimLayout->addWidget(dimBox,    0, 0, 1, 4);
    dimLayout->addWidget(depth,     3, 0, 1, 4);
    dimLayout->addWidget(dimcLeft,  4, 0, 1, 1);
    dimLayout->addWidget(dimc,      4, 1, 1, 2);
    dimLayout->addWidget(dimcRight, 4, 3, 1, 1);
    dimFrame = new QGroupBox("Dimensions");
    dimFrame->setLayout(dimLayout);

    resEdit = new QLineEdit("25");
    resLayout = new QHBoxLayout();
    resLayout->addWidget(resEdit);
    resFrame = new QGroupBox("Resolution (Pixels per Centimeter)");
    resFrame->setLayout(resLayout);


    redrawLabel = new QLabel(tr("Please wait. \nDrawing..."));

    //Creating the third column
    double cInc = 255.0/(phant.media.size());
    QString cStyle;

    med_colour_names.resize(phant.media.size());

    med_colors = new QVector <QPushButton *>;
    media_names = new QVector <QLabel *>;
    med_colors->resize(phant.media.size());
    media_names->resize(phant.media.size());
    medLayout = new QGridLayout();

    for (int i = 0; i < phant.media.size(); i++) {
        cStyle = "QPushButton {background-color: rgb(";
        cStyle += QString::number(int(cInc*(i+1)));
        cStyle += ",";
        cStyle += QString::number(int(cInc*(i+1)));
        cStyle += ",";
        cStyle += QString::number(int(cInc*(i+1)));
        cStyle += ")}";

        (*med_colors)[i] = new QPushButton();
        (*med_colors)[i]->setStyleSheet(cStyle);
        (*media_names)[i] = new QLabel(phant.media[i]);
        med_colour_names[i] = qRgb(int(cInc*(i)), int(cInc*(i)),int(cInc*(i)));
        medLayout->addWidget((*med_colors)[i],i,0);
        medLayout->addWidget((*media_names)[i],i,1);

    }

    // //Valid/assigned contours
    // out<<"contour name size " <<contourName.size() <<" unique size " <<structUnique.size() <<"\n";
    // QVector <QString> valid_contour;
    // for(int i=0; i<structUnique.size(); i++){
    // out<<contourName[i] <<"  " <<structUnique[i] <<"\n";
    // if(structUnique[i])
    // valid_contour.append(contourName[i]);
    // }

    // out<<valid_contour.size() <<"\n";
    // int num = valid_contour.size();

    // contour_colors = new QVector <QPushButton *>;
    // contour_name = new QVector <QLabel *>;
    // contour_colors->resize(num);
    // contour_name->resize(num);
    // contourLayout = new QGridLayout();

    // for (int i = 0; i < num; i++) {
    // (*contour_colors)[i] = new QPushButton();
    // cStyle = "QPushButton {background-color: rgb(";
    // cStyle += QString::number(int(255/num*(i+1)));
    // cStyle += ",0,";
    // cStyle += QString::number(int(255/num*(num-i-1)));
    // cStyle += ")}";
    // (*contour_colors)[i]->setStyleSheet(cStyle);
    // (*contour_name)[i] = new QLabel(valid_contour[i]);
    // contourLayout->addWidget((*contour_colors)[i],i,0);
    // contourLayout->addWidget((*contour_name)[i],i,1);
    // contour_colour_names[i] = qRgb(int(255/num*(i+1)), 0,int(255/num*(num-i-1)));
    // out<<valid_contour[i] <<"  " <<int(255/num*(i+1)) <<" " <<0 <<" " <<int(255/num*(num-i-1)) <<"\n";
    // }




    QLabel *medLabel = new QLabel(tr("Media"));
    //QLabel* contourLabel = new QLabel(tr("DICOM Contour"));
    medFrame = new QFrame;
    medFrame->setLayout(medLayout);
    //contourFrame = new QFrame;
    //contourFrame->setLayout(contLayout);

    pose = new QLabel("x: 0.000 cm y: 0.000 cm z: 0.000 cm");
    pose->setFont(QFont("Monospace", 10, QFont::Normal, false));



    // Column 1
    layout->addWidget(dimFrame, 0, 0);
    layout->addWidget(resFrame, 1, 0);
    layout->addWidget(savePic, 2, 0);
    layout->addWidget(redrawLabel, 3, 0);
    layout->setRowStretch(3,20);

    // Column 2-5
    layout->addWidget(imageFrame, 0, 1, 5, 4);
    layout->addWidget(pose, 5, 1);
    layout->addWidget(close, 5, 3);
    layout->setColumnStretch(2,20);

    // Column 6
    layout->addWidget(medLabel,0,5);
    layout->addWidget(medFrame, 1, 5);
    //layout->addWidget(contourLabel, 2, 5);
    // layout->addWidget(contourFrame1, 2, 5);

    window->setLayout(layout);
    window->setHidden(TRUE);
    window->setWindowTitle(tr("Egsphant Preview"));



    //Initializing bounds and vlaues
    ai = phant.x[0];
    af = phant.x[phant.nx];
    bi = phant.y[0];
    bf = phant.y[phant.ny];

    dimc->setRange(0,phant.nz-1);
    dimc->setSliderPosition(50);


    // Progress Bar
    remainder = new double (0.0);
    progWin = new QWidget();
    progLayout = new QGridLayout();
    progLabel = new QLabel();
    progLabel->setFont(QFont("Serif", 12, QFont::Normal, false));
    progress = new QProgressBar();
    progLayout->addWidget(progLabel, 0, 0);
    progLayout->addWidget(progress, 1, 0);
    progWin->setLayout(progLayout);
    progWin->resize(350, 0);
    progress->setRange(0, 1000000000);



    progWin->setFont(QFont("Serif", 12, QFont::Normal, false));


}

/***
Function: connectLayout
-----------------------

***/
void Preview::connectLayout() {

    connect(image, SIGNAL(mouseMoved(int, int)),
            this, SLOT(mouseGeom(int, int)));

    connect(dimBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeDim()));

    for (int i = 0; i < med_colors->size(); i++)
        connect((*med_colors)[i], SIGNAL(clicked()),
                this, SLOT(changeColor()));

    // for (int i = 0; i < contour_colors->size(); i++)
    // connect((*contour_colors)[i], SIGNAL(clicked()),
    // this, SLOT(changeColorContour()));

    connect(dimc, SIGNAL(sliderReleased()),         //worked when the  slider moves & not just clicked to left
            this, SLOT(redraw()));


    connect(dimcLeft, SIGNAL(clicked()),
            this, SLOT(changeDepthLeft()));

    connect(dimcRight, SIGNAL(clicked()),
            this, SLOT(changeDepthRight()));


    connect(resEdit, SIGNAL(editingFinished()),
            this, SLOT(redraw()));

    connect(savePic, SIGNAL(clicked()),
            this, SLOT(saveImage()));

    connect(close, SIGNAL(clicked()),
            this, SLOT(closePreview()));


}




/***
Function: close_preview
-----------------------
What happens with the closes pushed
***/
void Preview::close_preview() {
    this->setDisabled(TRUE);
    mom->setEnabled(TRUE);
    window->hide();

}



void Preview::changeDepthLeft() {
    int temp = 0;
    if ((temp = dimc->sliderPosition()) == dimc->minimum()) {
        return;
    }
    dimc->setSliderPosition(temp-1);
    redraw();
}

void Preview::changeDepthRight() {
    int temp = 0;
    if ((temp = dimc->sliderPosition()) == dimc->maximum()) {
        return;
    }
    dimc->setSliderPosition(temp+1);
    redraw();
}

/***
Function: changeColor
---------------------
Updates the phnaton colour of the contour
***/
void Preview::changeColor() {

    QPushButton *b = (QPushButton *)(QObject::sender());
    QPalette p(b->palette());
    QColor c = QColorDialog::getColor(p.color(QPalette::Button), 0,
                                      "Select Contour Color");
    QString cStyle;
    cStyle = "QPushButton {background-color: rgb(";
    cStyle += QString::number(c.red()) + ",";
    cStyle += QString::number(c.green()) + ",";
    cStyle += QString::number(c.blue()) + ")}";
    b->setStyleSheet(cStyle);

    QString colour;
    cStyle = QString::number(c.red()) + ",";
    cStyle += QString::number(c.green()) + ",";
    cStyle += QString::number(c.blue());


    for (int i = 0; i < med_colors->size(); i++) {

        QColor button_color = (*med_colors)[i]->palette().color(QPalette::Button);
        med_colour_names[i] = qRgb(int(button_color.red()), int(button_color.green()), int(button_color.blue()));

    }
    redraw();

}

// void Preview::changeColorContour()
// {
// QTextStream out(stdout);
// QPushButton* b = (QPushButton*)(QObject::sender());
// QPalette p (b->palette());
// QColor c = QColorDialog::getColor(p.color(QPalette::Button), 0,
// "Select Contour Color");
// QString cStyle;
// cStyle = "QPushButton {background-color: rgb(";
// cStyle += QString::number(c.red()) + ",";
// cStyle += QString::number(c.green()) + ",";
// cStyle += QString::number(c.blue()) + ")}";
// b->setStyleSheet(cStyle);

// QString colour;
// cStyle = QString::number(c.red()) + ",";
// cStyle += QString::number(c.green()) + ",";
// cStyle += QString::number(c.blue());


// for (int i = 0; i < contour_colors1->size(); i++){

// QColor button_color = (*contour_colors1)[i]->palette().color(QPalette::Button);
// contour_colour_names[i] = qRgb(int(button_color.red()), int(button_color.green()), int(button_color.blue()) );
// out<<i <<"  :" <<int(button_color.red()) <<" " <<int(button_color.green()) <<" " <<int(button_color.blue()) <<"\n";

// }
// redraw();

// }


/***
Function: changeDim
-------------------
Changes the boundaries and the phantom when the axis is changed
***/
void Preview::changeDim() {
    if (!dimBox->currentText().compare("x axis")) {
        depth->setText("x Depth");
        if (phant.nx && phant.ny && phant.nz) {
            ai = phant.y[0];
            af = phant.y[phant.ny];
            bi = phant.z[0];
            bf = phant.z[phant.nz];
            dimc->setRange(0,phant.nx-1);
        }
    }
    else if (!dimBox->currentText().compare("y axis")) {
        depth->setText("y Depth");
        if (phant.nx && phant.ny && phant.nz) {
            ai = phant.x[0];
            af = phant.x[phant.nx];
            bi = phant.z[0];
            bf = phant.z[phant.nz];
            dimc->setRange(0,phant.ny-1);
        }
    }
    else if (!dimBox->currentText().compare("z axis")) {
        depth->setText("z Depth");
        if (phant.nx && phant.ny && phant.nz) {
            ai= phant.x[0];
            af= phant.x[phant.nx];
            bi= phant.y[0];
            bf= phant.y[phant.ny];
            dimc->setRange(0,phant.nz-1);
        }
    }

    redraw();
}


/***
Function: redraw
----------------
Redraws the phantom slice according to the user selections
***/
void Preview::redraw() {
    QTextStream out(stdout);

    if (!(phant.nx && phant.ny && phant.nz)) {
        return;
    }

    out<<"Drawing..." <<endl;
    clock_t t = clock();

    //Disable the items and display text that says redrawing
    redrawLabel->setText(tr("Please wait. \nRedrawing..."));
    dimFrame->setDisabled(true);
    resFrame->setDisabled(true);
    savePic->setDisabled(true);
    medFrame->setDisabled(true);
    close->setDisabled(true);
    QApplication::processEvents();

    //Determining the image slice
    double depth = 0;
    if (!dimBox->currentText().compare("x axis")) {
        depth = (phant.x[dimc->value()]+phant.x[dimc->value()+1])/2.0;
    }
    else if (!dimBox->currentText().compare("y axis")) {
        depth = (phant.y[dimc->value()]+phant.y[dimc->value()+1])/2.0;
    }
    else if (!dimBox->currentText().compare("z axis")) {
        depth = (phant.z[dimc->value()]+phant.z[dimc->value()+1])/2.0;
    }

    //Getting the image
    // phantPicture = QPixmap::fromImage(
    // phant.getEGSPhantPicMed(dimBox->currentText(),
    // bi, bf, ai, af, depth,
    // resEdit->text().toInt()));

    phantPicture = QPixmap::fromImage(
                       getEGSPhantPicMed(dimBox->currentText(),
                                         bi, bf, ai, af, depth,
                                         resEdit->text().toInt()));
    //  if (typeMed->isChecked())
    //     phantPicture = QPixmap::fromImage(
    //                        phant.getEGSPhantPicMed(dimBox->currentText(),
    //                                dimbi->getText().toDouble(),
    //                                dimbf->getText().toDouble(),
    //                                dimai->getText().toDouble(),
    //                                dimaf->getText().toDouble(),
    //                                depth,
    //                                resEdit->text().toInt()));
    // else if (typeDen->isChecked())
    //     phantPicture = QPixmap::fromImage(
    //                        phant.getEGSPhantPicDen(dimBox->currentText(),
    //                                dimbi->getText().toDouble(),
    //                                dimbf->getText().toDouble(),
    //                                dimai->getText().toDouble(),
    //                                dimaf->getText().toDouble(),
    //                                depth,
    //                                resEdit->text().toInt()));

    *picture = phantPicture;
    updateImage();

    t = clock() - t;
    out<<"	Process took " <<(((float)t)/CLOCKS_PER_SEC) <<" sec" <<endl;

    redrawLabel->setText("");
    dimFrame->setEnabled(true);
    resFrame->setEnabled(true);
    savePic->setEnabled(true);
    medFrame->setEnabled(true);
    close->setEnabled(true);
    QApplication::processEvents();

}




/***
Function: getEGSPhantPicMed
---------------------------
Creats a QImage of the contours in the slice

Inputs: axis: The axis that your viewing the slize from
        ai: initial boundary (the axis of a depends on the axis of the slice)
        af: final boundary (the axis of a depends on the axis of the slice)
        bi: initial boundary (the axis of b depends on the axis of the slice)
        bf: final boundary (the axis of b depends on the axis of the slice)
        d: the depth/position of the slzie
        res: the number of pixels per centimer in the image
***/
QImage Preview::getEGSPhantPicMed(QString axis, double ai, double af,
                                  double bi, double bf, double d, int res) {

    // Create a temporary image
    int width  = (af-ai)*res;
    int height = (bf-bi)*res;
    QImage image(height, width, QImage::Format_RGB32);
    double hInc, wInc;
    double h, w;
    int c;

    // Calculate the size (in cm) of pixels, and then the range for grayscaling
    wInc = 1/double(res);
    hInc = 1/double(res);


    //itterating through each pixel of the image
    for (int i = height-1; i >= 0; i--)
        for (int j = 0; j < width; j++) {
            // determine the location of the current pixel in the phantom
            h = (double(bi)) + hInc * double(i);
            w = (double(ai)) + wInc * double(j);

            // get the media, which differs based on axis through which image is
            // sliced
            if (!axis.compare("x axis")) {
                c = getMedia(d, h, w);
            }
            else if (!axis.compare("y axis")) {
                c = getMedia(h, d, w);
            }
            else if (!axis.compare("z axis")) {
                c = getMedia(h, w, d);
            }

            //out<<c <<" ";
            // finally, paint the pixel
            image.setPixelColor(i, j, med_colour_names[c]);

        }
    //out<<"\n";
    return image; // returns the image created

}



// get_idx_from_ijk takes and (i,j,k) tuple of a 3ddose distribution
// and returns the voxel index
int Preview::get_idx_from_ijk(int ii, int jj, int kk) {
    return ii+jj*phant.nx+kk*phant.nx*phant.ny;
}



/***
Function: getMedia
------------------
Retuns the index fo the contour media at the coordinates
Inputs: px: x-position
        py: y-position
        pz: z-position
***/
int Preview::getMedia(double px, double py, double pz) {
    int ix, iy, iz;
    ix = iy = iz = -1;

    // Find the index of the boundary that is less than px, py and pz
    for (int i = 0; i < phant.nx; i++)
        if (px <= phant.x[i+1]) {
            ix = i;
            break;
        }
    if (px < phant.x[0]) {
        ix = -1;
    }

    for (int i = 0; i < phant.ny; i++)
        if (py <= phant.y[i+1]) {
            iy = i;
            break;
        }
    if (py < phant.y[0]) {
        iy = -1;
    }

    for (int i = 0; i < phant.nz; i++)
        if (pz <= phant.z[i+1]) {
            iz = i;
            break;
        }
    if (pz < phant.z[0]) {
        iz = -1;
    }


    // This is to ensure that no area outside the vectors is accessed
    if (ix < phant.nx && ix >= 0 && iy < phant.ny && iy >= 0 && iz < phant.nz && iz >= 0) {
        return medCharMap[phant.m[ix][iy][iz]];
    }

    return 0; // We are not within our bounds
}


/***
Function: updateImage
---------------------
Updates the phantom image
***/
void Preview::updateImage() {
    image->setPixmap(*picture);
    image->setFixedSize(picture->width(), picture->height());
    image->repaint();

}


/***
Function: saveImage
---------------------
Allows the user to save the image
***/
void Preview::saveImage() {
    this->setDisabled(true);
    QString path = QFileDialog::getSaveFileName(0, tr("Save File"), "",
                   tr("PNG (*.png)"));
    if (path == "") {
        return;
    }
    if (!path.endsWith(".png")) {
        path += ".png";
    }

    picture->save(path, "PNG");
    this->setEnabled(true);
}


/***
Function: closePreview
---------------------
Closes the previewer window
***/
void Preview::closePreview() {
    this->setDisabled(TRUE);
    mom->setEnabled(TRUE);
    window->hide();

    emit closed();
}


/***
Function: updateProgress
------------------------
Updates the progress bar by the increment, n
***/
void Preview::updateProgress(double n) {

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
Function: mouseGeom
--------------------
Determines the x,y,z position of the mouse on the image
Inputs: width: the x-position of the mouse in the image pane
        height: the y-position of the mouse in the image pane
***/
void Preview::mouseGeom(int width, int height) {
    QString temp = "";
    double min, res;
    QString axis = dimBox->currentText();
    res = resEdit->text().toInt();
    min = ai;
    double a = width/res + min;
    min = bi;
    double b = min + height/res;
    double c;

    // Create the proper label for the proper axis
    if (!axis.compare("x axis")) {
        c = (phant.x[dimc->value()]+phant.x[dimc->value()+1])/2.0;
        temp += "x: ";
        temp += QString::number(c, 'f', 3);
        temp += " cm y: ";
        temp += QString::number(a, 'f', 3);
        temp += " cm z: ";
        temp += QString::number(b, 'f', 3);
        temp += " cm";
    }
    else if (!axis.compare("y axis")) {
        c = (phant.y[dimc->value()]+phant.y[dimc->value()+1])/2.0;
        temp += "x: ";
        temp += QString::number(a, 'f', 3);
        temp += " cm y: ";
        temp += QString::number(c, 'f', 3);
        temp += " cm z: ";
        temp += QString::number(b, 'f', 3);
        temp += " cm";
    }
    else if (!axis.compare("z axis")) {
        c = (phant.z[dimc->value()]+phant.z[dimc->value()+1])/2.0;
        temp += "x: ";
        temp += QString::number(a, 'f', 3);
        temp += " cm y: ";
        temp += QString::number(b, 'f', 3);
        temp += " cm z: ";
        temp += QString::number(c, 'f', 3);
        temp += " cm";
    }

    pose->setText(temp);    //sets the x,y,z position to the label
    pose->repaint();
}



/***
Function: create_progress_bar
------------------------------
***/
void Preview::create_progress_bar() {

    // Set up the progress bar
    progress->reset();
    *remainder = 0;
    progWin->setWindowTitle("Creating Patient Model Image");
    progLabel->setText("Redrawing...");
    progWin->show();
    progWin->activateWindow();
    progWin->raise();

}















