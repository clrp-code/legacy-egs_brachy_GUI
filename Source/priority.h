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

#ifndef PRIORITY_HH
#define PRIORITY_HH

#define TRUE 1
#define FALSE 0

#include <QApplication>
#include <QtWidgets>
#include <iostream>
#include <math.h>

class priority: public QWidget {
private:
    Q_OBJECT


    QVector<QString> Names;
    QStringList orderedNames;
    QVector<bool> structInclude;
    QMap <int, QString> structOrder;

signals:
    void closed();
    void exit();

protected:
    void closeEvent(QCloseEvent *event) override;


public:
    //Constructor
    priority(QWidget *parent, QVector<QString> structName, QVector<bool> structUnique);
    //Decnstructor
    ~priority();

    void createLayout();
    void connectLayout();

    QVector <int> structPrio;


//Tissue assingment scheme window------------------------------------------------
    QWidget *window;
    QGridLayout *tissLayout;
    QListWidget *prio_selection;
    QLabel *label;

    //Data
    QWidget *mom;

    bool closed_check = false;


    QScrollArea *scrollArea;
    QPushButton *apply;
    QPushButton *cancel;
    QGridLayout *layout;


public slots:
    void close_tissue_check();
    void cancel_window();


};








#endif