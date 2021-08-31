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

#include "parse_dicom.h"

//#define OUTPUT_ALL
//#define OUTPUT_TAG
//#define OUTPUT_SQ
#define MAX_DATA_PRINT 0 // 0 means any size

Attribute::Attribute() {
    vf = NULL; // This stops seg faults when calling the destructor below
}

Attribute::~Attribute() {
    if (vf != NULL) {
        delete[] vf;
    }
}

SequenceItem::SequenceItem(unsigned long int size, unsigned char *data) {
    vl = size;
    vf = data;
}

SequenceItem::~SequenceItem() {
    if (vf != NULL) {
        delete[] vf;
    }
}

Sequence::~Sequence() {
    for (int i = 0; i < items.size(); i++) {
        delete items[i];
    }
    items.clear();
}

DICOM::DICOM(database *l) {
    lib = l;
    isImplicit = isBigEndian = false;
}

DICOM::DICOM() {
    isImplicit = isBigEndian = false;
}

DICOM::~DICOM() {
    // for (int i = 0; i < data.size(); i++) {
    // delete data[i];
    // }
    // data.clear();
}

int DICOM::readSequence(QDataStream *in, Attribute *att) {
    bool flag = true;
    int depth = 0;
    unsigned int *tag, size;
    unsigned char *dat;
    while (flag) {
        tag = new unsigned int;
        if (in->readRawData((char *)tag, 4) != 4) {
            // Not a DICOM file
            delete tag;
            return 0;
        }
        if (in->readRawData((char *)(&size), 4) != 4) {
            // Not a DICOM file
            delete tag;
            return 0;
        }

        if (*tag == (unsigned int)0xE0DDFFFE) { // sequence delimiter
            flag = false;
        }
        else if (size != (unsigned int)0xFFFFFFFF) {
            // sequence item with defined size
            dat = new unsigned char[size];
            if ((unsigned int)in->readRawData((char *)dat, size) != size) {
                // Not a DICOM file
                delete tag;
                delete[] dat;
                return 0;
            }
            att->seq.items.append(new SequenceItem(size, dat));
        }
        else if (size == (unsigned int)0xFFFFFFFF) {
            // sequence item with undefined size
            QByteArray buffer;
            depth = 0;
            dat = new unsigned char[1];
            while (true) { // Keeping appending data one char at a time...
                if (in->readRawData((char *)dat, 1) != 1) {
                    std::cout<<dat <<"reading data \n";
                    //Not a DICOM file
                    delete tag;
                    delete[] dat;
                    return 0;
                }

                buffer.append(*dat);

                // Is there a subsequence we are parsing
                if (buffer.endsWith("ÿÿÿÿ")) { //FFFF FFFF
                    if (!isImplicit) {
                        if (buffer.endsWith("SQ\0\0ÿÿÿÿ")) {
                            depth++; // Increase depth to skip delimiters until we exit subsequence
                        }
                    }
                    else {
                        unsigned short int tag[2];
                        tag[0] = *(unsigned short int *)(buffer.right(8).left(2).data());
                        tag[1] = *(unsigned short int *)(buffer.right(6).left(2).data());
                        Reference nearest = lib->binSearch(tag[0], tag[1], 0, lib->lib.size()-1);
                        if (nearest.tag[0] == tag[0] && nearest.tag[1] == tag[1] && !nearest.vr.compare("SQ")) {
                            depth++; // Increase depth to skip delimiters until we exit subsequence
                        }
                    }
                }

                // ...until we reach the sequence item delimiter
                if (*((unsigned int *)buffer.right(4).data()) == (unsigned int)0xE00DFFFE && !depth) {
                    buffer.chop(4);
                    delete[] dat;
                    dat = new unsigned char[4];
                    if (in->readRawData((char *)dat, 4) != 4) {
                        // Not a DICOM file
                        delete tag;
                        delete[] dat;
                        return 0;
                    }
                    delete[] dat;
                    dat = new unsigned char[buffer.size()];
                    for (int i = 0; i < buffer.size(); i++) {
                        dat[i] = buffer[i];
                    }
                    att->seq.items.append(new SequenceItem(buffer.size(),dat));
                    break;
                }
                else if (*((unsigned int *)buffer.right(4).data()) == (unsigned int)0xE0DDFFFE) {
                    depth--;
                }
            }
        }

        delete tag;
    }
    return 1;
}

int DICOM::readDefinedSequence(QDataStream *in, Attribute *att, unsigned long int n) {
    int depth = 0;
    unsigned int *tag, size;
    unsigned char *dat;
    while (n > 0) {
        tag = new unsigned int;
        if (in->readRawData((char *)tag, 4) != 4) {
            // Not a DICOM file
            delete tag;
            return 0;
        }
        if (in->readRawData((char *)(&size), 4) != 4) {
            // Not a DICOM file
            delete tag;
            return 0;
        }
        n-=8;

        if (size != (unsigned int)0xFFFFFFFF) {
            // sequence item with defined size
            dat = new unsigned char[size];
            if ((unsigned int)in->readRawData((char *)dat, size) != size) {
                // Not a DICOM file
                delete tag;
                delete[] dat;
                return 0;
            }
            att->seq.items.append(new SequenceItem(size, dat));
            n-=size;
        }
        else if (size == (unsigned int)0xFFFFFFFF) {
            // sequence item with undefined size
            QByteArray buffer;
            dat = new unsigned char[1];
            while (true) { // Keeping appending data one char at a time...
                if (in->readRawData((char *)dat, 1) != 1) {
                    // Not a DICOM file
                    delete tag;
                    delete[] dat;
                    return 0;
                }
                buffer.append(*dat);

                // Is there a subsequence we are parsing
                if (buffer.endsWith("ÿÿÿÿ")) {
                    if (!isImplicit) {
                        if (buffer.endsWith("SQ\0\0ÿÿÿÿ")) {
                            depth++; // Increase depth to skip delimiters until we exit subsequence
                        }
                    }
                    else {
                        unsigned short int tag[2];
                        tag[0] = *(unsigned short int *)(buffer.right(8).left(2).data());
                        tag[1] = *(unsigned short int *)(buffer.right(6).left(2).data());
                        Reference nearest = lib->binSearch(tag[0], tag[1], 0, lib->lib.size()-1);
                        if (nearest.tag[0] == tag[0] && nearest.tag[1] == tag[1] && !nearest.vr.compare("SQ")) {
                            depth++; // Increase depth to skip delimiters until we exit subsequence
                        }
                    }
                }

                // ...until we reach the sequence item delimiter
                if (*((unsigned int *)buffer.right(4).data()) == (unsigned int)0xE00DFFFE && !depth) {
                    delete[] dat;
                    dat = new unsigned char[4];
                    if (in->readRawData((char *)dat, 4) != 4) {
                        // Not a DICOM file
                        delete tag;
                        delete[] dat;
                        return 0;
                    }
                    delete[] dat;
                    dat = new unsigned char[buffer.size()];
                    for (int i = 0; i < buffer.size(); i++) {
                        dat[i] = buffer[i];
                    }
                    att->seq.items.append(new SequenceItem(buffer.size(),dat));
                    n-=buffer.size();
                    break;
                }
                else if (*((unsigned int *)buffer.right(4).data()) == (unsigned int)0xE0DDFFFE) {
                    depth--;
                }
            }
        }

        delete tag;
    }
    return 1;
}

int DICOM::parse(QString p) {
    path = p;
    QFile file(path);
    int k = 0, l = 0;
    if (file.open(QIODevice::ReadOnly)) {
        unsigned char *dat;
        QDataStream in(&file);
        in.setByteOrder(QDataStream::LittleEndian);

        /*============================================================================*/
        /*DICOM HEADER READER=========================================================*/
        // Skip the first bit of white space in DICOM
        dat = new unsigned char[128];
        if (in.readRawData((char *)dat, 128) != 128) {
            // Not a DICOM file
            delete[] dat;
            file.close();
            return 0;
        }
        delete[] dat;

        // Read in DICM characters at start of file
        dat = new unsigned char[4];
        if (in.readRawData((char *)dat, 4) != 4) {
            // Not a DICOM file
            delete[] dat;
            file.close();
            return 0;
        }
        else if ((QString(dat[0])+dat[1]+dat[2]+dat[3]) != "DICM") {
            // Not a DICOM file
            delete[] dat;
            file.close();
            return 0;
        }
        delete[] dat;

        /*============================================================================*/
        /*BEGINNING OF DATA ELEMENT READING LOOP======================================*/
        Attribute *temp;
        unsigned int size;
        QString VR;
        bool nested = false;
        while (!in.atEnd()) {
            temp = new Attribute();

            /*============================================================================*/
            /*RETRIEVE ELEMENT TAG========================================================*/
            // Get the tag
            k++; // iterate
#if defined(OUTPUT_ALL) || defined(OUTPUT_TAG)
            std::cout << std::dec << k << ") " << "Tag ";
#endif
            dat = new unsigned char[4];
            if (in.readRawData((char *)dat,4) != 4) {
                // Not a DICOM file
                delete[] dat;
                file.close();
                return 0;
            }
            temp->tag[0]= ((unsigned short int)(dat[1]) << 8) +
                          (unsigned short int)dat[0];
            temp->tag[1]= ((unsigned short int)(dat[3]) << 8) +
                          (unsigned short int)dat[2];
#if defined(OUTPUT_ALL) || defined(OUTPUT_TAG)
            std::cout << std::hex << temp->tag[0] << ","
                      <<  temp->tag[1] << " | Representation ";
#endif
            delete[] dat;

            if (temp->tag[0] == 0xFFFE && (temp->tag[1] == 0xE0DD || temp->tag[1] == 0xE00D)) {
                // Not a DICOM file
                std::cout << "Misreading sequence delimiters as top level data elements, something has gone wrong \n";
                //delete[] dat;
                //file.close();
                //return 0;
            }

            /*============================================================================*/
            /*NON-NESTED PROCEDURE: GET VR, SIZE AND DATA=================================*/
            // Normal data elements
            // Get the VR
            dat = new unsigned char[4];

            if (!isImplicit || temp->tag[0] == 0x0002) {
                if (in.readRawData((char *)dat,4) != 4) {
                    // Not a DICOM file
                    delete[] dat;
                    file.close();
                    return 0;
                }

                VR = QString(dat[0])+dat[1];

#if defined(OUTPUT_ALL) || defined(OUTPUT_TAG)
                std::cout << ((unsigned short int)(dat[0]) << 8) +
                          (unsigned short int)dat[1]
                          << " -> " << VR.toStdString() << " | Size ";
#endif
            }
            else {
                VR = lib->binSearch(temp->tag[0], temp->tag[1], 0, lib->lib.size()-1).vr;
#if defined(OUTPUT_ALL) || defined(OUTPUT_TAG)
                std::cout << VR.toStdString() << " (implicit) | Size ";
#endif
            }

            // Get size
            if ((temp->tag[0] != 0x0002 && isImplicit) || (lib->implicitVR.contains(VR))) {
                if (in.readRawData((char *)dat,4) != 4) { //Reread for size
                    // Not a DICOM file
                    delete[] dat;
                    file.close();
                    return 0;
                }
                temp->vl = ((unsigned int)(dat[3]) << 24) +
                           ((unsigned int)(dat[2]) << 16) +
                           ((unsigned int)(dat[1]) << 8) +
                           (unsigned int)dat[0];

                // We have a sequence
                if (!VR.compare("SQ") && temp->vl == (unsigned int)0xFFFFFFFF) {
                    nested = true;
                    if (!readSequence(&in, temp)) {
                        return 0;
                    }
                }
                else if (!VR.compare("SQ")) {
                    nested = true;
                    if (!readDefinedSequence(&in, temp, temp->vl)) {
                        return 0;
                    }
                }
            }
            else {
                if (isImplicit && temp->tag[0] != 0x0002)
                    if (in.readRawData((char *)dat,4) != 4) {
                        // Not a DICOM file
                        delete[] dat;
                        file.close();
                        return 0;
                    }

                if (lib->validVR.contains(VR))
                    temp->vl = ((unsigned short int)(dat[3]) << 8) +
                               (unsigned short int)dat[2];
                else
                    temp->vl = ((unsigned int)(dat[3]) << 24) +
                               ((unsigned int)(dat[2]) << 16) +
                               ((unsigned int)(dat[1]) << 8) +
                               (unsigned int)dat[0];

                // We have a sequence
                if (!VR.compare("SQ") && temp->vl == (unsigned int)0xFFFFFFFF) {
                    nested = true;
                    if (!readSequence(&in, temp)) {
                        return 0;
                    }
                }
                else if (!VR.compare("SQ")) {
                    nested = true;
                    if (!readDefinedSequence(&in, temp, temp->vl)) {
                        return 0;
                    }
                }
            }

            if (temp->vl == (unsigned int)0xFFFFFFFF) {
                temp->vl = 0;
            }

            size = temp->vl;

#if defined(OUTPUT_ALL) || defined(OUTPUT_TAG)
            std::cout << temp->vl << " -> " << std::dec << size << "\n";
#endif

            Reference closest = lib->binSearch(temp->tag[0], temp->tag[1], 0, lib->lib.size()-1);
            if (closest.tag[0] == temp->tag[0] && closest.tag[1] == temp->tag[1]) {
                temp->desc = closest.title;
                l++;
            }
            else {
                temp->desc = "Unknown Tag";
            }

#ifdef OUTPUT_ALL
            std::cout << temp->desc.toStdString() << ": ";
#endif
            delete[] dat;

            // Get data
            if (!nested) {
                temp->vf = new unsigned char[size];
                if (size > 0 && size < (unsigned long int)INT_MAX) {
                    if (in.readRawData((char *)temp->vf,size) != (long int)size) {
                        // Not a DICOM file
                        file.close();
                        return 0;
                    }
                }
                else if (size > 0) { // In case we need to read in more data
                    // then the buffer can handle
                    unsigned char *pt = temp->vf;
                    for (int i = 0; (unsigned long int)i <
                            size/((unsigned long int)INT_MAX); i++) {
                        if (in.readRawData((char *)pt,INT_MAX) != INT_MAX) {
                            // Not a DICOM file
                            file.close();
                            return 0;
                        }
                        pt += sizeof(char)*INT_MAX;
                    }
                }

#ifdef OUTPUT_ALL
                unsigned long int avoidWarning =
                    (unsigned long int)MAX_DATA_PRINT;
                if (avoidWarning == 0 || size < avoidWarning)
                    // It's a string
                    if (!VR.compare("UI") || !VR.compare("SH") || !VR.compare("AE") || !VR.compare("DA") ||
                            !VR.compare("TM") || !VR.compare("LO") || !VR.compare("ST") || !VR.compare("PN") ||
                            !VR.compare("DT") || !VR.compare("LT") || !VR.compare("UT") || !VR.compare("IS") ||
                            !VR.compare("OW") || !VR.compare("DS") || !VR.compare("CS") || !VR.compare("AS"))
                        for (unsigned long int i = 0; i < size; i++) {
                            std::cout << dat[i];
                        }
                // It's a tag
                    else if (!VR.compare("AT"))
                        std::cout << ((unsigned int)(dat[3]) << 24) +
                                  ((unsigned int)(dat[2]) << 16) +
                                  ((unsigned int)(dat[1]) << 8) +
                                  (unsigned int)(dat[0]);
                    else if (!VR.compare("FL"))
                        if (isBigEndian)
                            std::cout << std::dec << float(((int)(dat[0]) << 24) +
                                                           ((int)(dat[1]) << 16) +
                                                           ((int)(dat[2]) << 8) +
                                                           (int)(dat[3])) << std::hex;
                        else
                            std::cout << std::dec << float(((int)(dat[3]) << 24) +
                                                           ((int)(dat[2]) << 16) +
                                                           ((int)(dat[1]) << 8) +
                                                           (int)(dat[0])) << std::hex;
                    else if (!VR.compare("FD"))
                        if (isBigEndian)
                            std::cout << std::dec << double(((long int)(dat[0]) << 56) +
                                                            ((long int)(dat[1]) << 48) +
                                                            ((long int)(dat[2]) << 40) +
                                                            ((long int)(dat[3]) << 32) +
                                                            ((long int)(dat[4]) << 24) +
                                                            ((long int)(dat[5]) << 16) +
                                                            ((long int)(dat[6]) << 8) +
                                                            (long int)(dat[7])) << std::hex;
                        else
                            std::cout << std::dec << double(((long int)(dat[7]) << 56) +
                                                            ((long int)(dat[6]) << 48) +
                                                            ((long int)(dat[5]) << 40) +
                                                            ((long int)(dat[4]) << 32) +
                                                            ((long int)(dat[3]) << 24) +
                                                            ((long int)(dat[2]) << 16) +
                                                            ((long int)(dat[1]) << 8) +
                                                            (long int)(dat[0])) << std::hex;
                    else if (!VR.compare("SL"))
                        if (isBigEndian)
                            std::cout << std::dec << (((int)(dat[0]) << 24) +
                                                      ((int)(dat[1]) << 16) +
                                                      ((int)(dat[2]) << 8) +
                                                      (int)(dat[3])) << std::hex;
                        else
                            std::cout << std::dec << (((int)(dat[3]) << 24) +
                                                      ((int)(dat[2]) << 16) +
                                                      ((int)(dat[1]) << 8) +
                                                      (int)(dat[0])) << std::hex;
                    else if (!VR.compare("SS"))
                        if (isBigEndian)
                            std::cout << std::dec << (((short int)(dat[0]) << 8) +
                                                      (short int)(dat[1])) << std::hex;
                        else
                            std::cout << std::dec << (((short int)(dat[1]) << 8) +
                                                      (short int)(dat[0])) << std::hex;
                    else if (!VR.compare("UL"))
                        if (isBigEndian)
                            std::cout << std::dec << (unsigned int)(((int)(dat[0]) << 24) +
                                                                    ((int)(dat[1]) << 16) +
                                                                    ((int)(dat[2]) << 8) +
                                                                    (int)(dat[3])) << std::hex;
                        else
                            std::cout << std::dec << (unsigned int)(((int)(dat[3]) << 24) +
                                                                    ((int)(dat[2]) << 16) +
                                                                    ((int)(dat[1]) << 8) +
                                                                    (int)(dat[0])) << std::hex;
                    else if (!VR.compare("US"))
                        if (isBigEndian)
                            std::cout << std::dec << (unsigned short int)(((short int)(dat[0]) << 8) +
                                      (short int)(dat[1])) << std::hex;
                        else
                            std::cout << std::dec << (unsigned short int)(((short int)(dat[1]) << 8) +
                                      (short int)(dat[0])) << std::hex;
                    else if (!VR.compare("SQ")) {
                        std::cout << "Sequence printed as strings below";
                    }
                    else {
                        std::cout << "Unsupported format";
                    }
                else
                    std::cout << "Data larger than " << std::dec
                              << avoidWarning << std::hex;
                std::cout << "\n";
#endif
#if defined(OUTPUT_ALL) || defined(OUTPUT_TAG)
                std::cout << std::dec << "\n";
#endif

                // Save proper transfer syntax for farther parsing
                if (temp->tag[0] == 0x0002 && temp->tag[1] == 0x0010) {
                    QString TransSyntax((char *)temp->vf);
                    if (!TransSyntax.compare("1.2.840.10008.1.2.1")) {
                        isImplicit = false;
                        isBigEndian = false;
                    }
                    else if (!TransSyntax.compare("1.2.840.10008.1.2.2")) {
                        isImplicit = false;
                        isBigEndian = true;
                    }
                    else if (!TransSyntax.compare("1.2.840.10008.1.2")) {
                        isImplicit = true;
                        isBigEndian = false;
                    }
                    else {
                        std::cout << "Unknown transfer syntax, assuming explicit and little endian\n";
                        isImplicit = false;
                        isBigEndian = false;
                    }
                }

                // Save slice height for later sorting
                if (temp->tag[0] == 0x0020 && temp->tag[1] == 0x0032) {
                    QString tempS = "";
                    for (unsigned int s = 0; s < temp->vl; s++) {
                        tempS.append(temp->vf[s]);
                    }

                    z = (tempS.split('\\',QString::SkipEmptyParts)[2]).toDouble();
                }
            }
            else if (nested) {
#if defined(OUTPUT_ALL) || defined(OUTPUT_TAG)
                std::cout << "Nested data\n";
                for (int i = 0; i < temp->seq.items.size(); i++) {
                    std::cout << "\t" << std::dec << i+1 << ") ";
                    unsigned long int avoidWarning = (unsigned long int)MAX_DATA_PRINT;
                    if (avoidWarning == 0 || temp->seq.items[i]->vl < avoidWarning)
                        for (unsigned int j = 0; j < temp->seq.items[i]->vl; j++) {
                            std::cout << std::hex << (*(temp->seq.items[i])).vf[j];
                        }
                    else {
                        std::cout << "Data larger than " << std::dec << avoidWarning << std::hex;
                    }

                    std::cout << std::hex << "\n";
                }
                std::cout << "\n";
#endif
                nested = false;
            }
            data.append(temp);
            /*============================================================================*/
            /*REPEAT UNTIL EOF============================================================*/
        }
        file.close();
        return l;
    }
    return 0;
}

int DICOM::parseSequence(QDataStream *in, QVector <Attribute *> *att) {
    unsigned char *dat;
    in->setByteOrder(QDataStream::LittleEndian);
    Attribute *temp;
    unsigned int size;
    QString VR;
    bool nested = false;
#if defined(OUTPUT_SQ)
    std::cout << "\nEntering the parsing loop\n";
    std::cout.flush();
#endif
    while (!in->atEnd()) {
        nested = false;
        temp = new Attribute();

        // Get the tag
        dat = new unsigned char[4];
        if (in->readRawData((char *)dat,4) != 4) {
            // Not a DICOM file
            delete[] dat;
            delete temp;
            return 1;
        }
#if defined(OUTPUT_SQ)
        std::cout << "Tag ";
        std::cout.flush();
#endif

        temp->tag[0]= ((unsigned short int)(dat[1]) << 8) +
                      (unsigned short int)dat[0];
        temp->tag[1]= ((unsigned short int)(dat[3]) << 8) +
                      (unsigned short int)dat[2];
        delete[] dat;
#if defined(OUTPUT_SQ)
        std::cout << std::hex << temp->tag[0] << "," <<  temp->tag[1] << " | Representation " << std::dec;
        std::cout.flush();
#endif

        // Get the VR
        dat = new unsigned char[4];
        if (!isImplicit || temp->tag[0] == 0x0002) {
            if (in->readRawData((char *)dat,4) != 4) {
                // Not a DICOM file
                delete[] dat;
                delete temp;
                return 0;
            }

            VR = QString(dat[0])+dat[1];
#if defined(OUTPUT_SQ)
            std::cout << ((unsigned short int)(dat[0]) << 8) + (unsigned short int)dat[1] << " -> " << VR.toStdString() << " | Size ";
            std::cout.flush();
#endif
        }
        else {
            VR = lib->binSearch(temp->tag[0], temp->tag[1], 0, lib->lib.size()-1).vr;
#if defined(OUTPUT_SQ)
            std::cout << VR.toStdString() << " (implicit) | Size ";
            std::cout.flush();
#endif
        }

        // Get size
        if ((temp->tag[0] != 0x0002 && isImplicit) || (lib->implicitVR.contains(VR))) {
            if (in->readRawData((char *)dat,4) != 4) { //Reread for size
                // Not a DICOM file
                delete[] dat;
                delete temp;
                return 0;
            }
            temp->vl = ((unsigned int)(dat[3]) << 24) +
                       ((unsigned int)(dat[2]) << 16) +
                       ((unsigned int)(dat[1]) << 8) +
                       (unsigned int)dat[0];

            // We have a sequence
            if (!VR.compare("SQ") && temp->vl == (unsigned int)0xFFFFFFFF) {
                nested = true;
                if (!readSequence(in, temp)) {
                    delete[] dat;
                    delete temp;
                    return 0;
                }
            }
            else if (!VR.compare("SQ")) {
                nested = true;
                if (!readDefinedSequence(in, temp, temp->vl)) {
                    delete[] dat;
                    delete temp;
                    return 0;
                }
            }
        }
        else {
            if (isImplicit && temp->tag[0] != 0x0002)
                if (in->readRawData((char *)dat,4) != 4) {
                    // Not a DICOM file
                    delete[] dat;
                    delete temp;
                    return 0;
                }

            if (lib->validVR.contains(VR))
                temp->vl = ((unsigned short int)(dat[3]) << 8) +
                           (unsigned short int)dat[2];
            else
                temp->vl = ((unsigned int)(dat[3]) << 24) +
                           ((unsigned int)(dat[2]) << 16) +
                           ((unsigned int)(dat[1]) << 8) +
                           (unsigned int)dat[0];

            // We have a sequence
            if (!VR.compare("SQ") && temp->vl == (unsigned int)0xFFFFFFFF) {
                nested = true;
                if (!readSequence(in, temp)) {
                    delete[] dat;
                    delete temp;
                    return 0;
                }
            }
            else if (!VR.compare("SQ")) {
                nested = true;
                if (!readDefinedSequence(in, temp, temp->vl)) {
                    delete[] dat;
                    delete temp;
                    return 0;
                }
            }
        }

        if (temp->vl == (unsigned int)0xFFFFFFFF) {
            temp->vl = 0;
        }

        size = temp->vl;
#if defined(OUTPUT_SQ)
        std::cout << temp->vl << " -> " << std::dec << size << "\n";
#endif

        Reference closest = lib->binSearch(temp->tag[0], temp->tag[1], 0, lib->lib.size()-1);
        if (closest.tag[0] == temp->tag[0] && closest.tag[1] == temp->tag[1]) {
            temp->desc = closest.title;
        }
        else {
            temp->desc = "Unknown Tag";
        }

        delete[] dat;

        // Get data
        if (!nested) {
            temp->vf = new unsigned char[size];
            if (size > 0 && size < (unsigned long int)INT_MAX) {
                if (in->readRawData((char *)temp->vf,size) != (long int)size) {
                    // Not a DICOM file
                    delete temp;
                    return 0;
                }
            }
            else if (size > 0) { // In case we need to read in more data then the buffer can handle
                unsigned char *pt = temp->vf;
                for (int i = 0; (unsigned long int)i < size/((unsigned long int)INT_MAX); i++) {
                    if (in->readRawData((char *)pt,INT_MAX) != INT_MAX) {
                        // Not a DICOM file
                        delete temp;
                        return 0;
                    }
                    pt += sizeof(char)*INT_MAX;
                }
            }
        }

#if defined(OUTPUT_SQ)
        if (!nested) {
            QString tempS = "";
            for (unsigned int s = 0; s < temp->vl; s++) {
                tempS.append(temp->vf[s]);
            }
            std::cout << tempS.toStdString() << "\n";
        }
        else {
            for (int i = 0; i < temp->seq.items.size(); i++) {
                std::cout << i << ") ";
                for (unsigned int j = 0; j < temp->seq.items[i]->vl; j++) {
                    std::cout << (*(temp->seq.items[i])).vf[j];
                }
                std::cout << "\n";
            }
        }
#endif

        att->append(temp);
    }
    return att->size();
}

double Interface::interp(double x, double x1, double x2, double y1, double y2) {
    return (y2*(x-x1)+y1*(x2-x))/(x2-x1);
}

void DICOM::submerge(QVector <DICOM *> &data, int i, int c, int f) {
    // We have three indices, l for one subsection, r the other, and j for the new sorted array
    int l = i, r = c+1, j = 0;
    QVector <DICOM *> temp(f-i+1); // Set aside memory for sorted array

    // While we have yet to iterate through either subsection
    while (l <= c && r <= f) {
        // If value at r index is smaller then add it to temp and move to next r
        if (data[l]->z > data[r]->z) {
            temp[j++] = data[r++];
        }
        // If value at l index is smaller then add it to temp and move to next l
        else {
            temp[j++] = data[l++];
        }
    }

    // Add all the remaining elements
    while (r <= f) {
        temp[j++] = data[r++];
    }
    while (l <= c) {
        temp[j++] = data[l++];
    }

    // Reassign all the data values to the temp values
    for (int k = 0; k < j; k++) {
        data[i+k] = temp[k];
    }
}

void DICOM::mergeSort(QVector <DICOM *> &data, int n) {
    // If our array is size 1 or less quit
    if (n <= 1) {
        return;
    }

    int subn = 1; // subn the size of subsections that are being submerged
    int i = 0; // i is the current index of the array at which we are at

    // While we are still submerging sections that are smaller than the size of the array
    while (subn < n) {
        // Reset the index to 0
        i = 0;

        // Iterate through n/(2*subn) sections, truncated
        while (i < n - subn) {

            // submerge two subn sized portions of data of the array
            if (i + (2 * subn) < n) {
                submerge(data, i, i + subn - 1, i + 2 * subn - 1);
            }

            // Or submerge a subn sized section and whatever is left of the array
            else {
                submerge(data, i, i + subn - 1, n - 1);
            }

            // Move the index to submerge the next 2 subsections
            i += 2 * subn;
        }

        // Double the size of subsection to be merged
        subn *= 2;
    }
}

void DICOM::create_progress_bar_dicom() {
    // Progress Bar ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    this->setDisabled(true);
    remainder2 = new double (0.0);
    progWin2 = new QWidget();
    progLayout2 = new QGridLayout();
    progress2 = new QProgressBar();
    progLabel2 = new QLabel();
    progLayout2->addWidget(progress2, 0, 0);
    progWin2->setLayout(progLayout2);
    progWin2->resize(300, 0);
    progress2->setRange(0, 1000000000);

}

/***
Destructor
***/
void DICOM::destruct_progress_bar_dicom() {
    delete remainder2;
    delete progress2;
    delete progLabel2;
    delete progLayout2;
    delete progWin2;

}



Interface::Interface() {
    createLayout();
    connectLayout();

}

// Destructor
Interface::~Interface() {

    delete select_location_phantom;
    delete select_location;

    delete DICOMButton;
    delete CalibButton;
    delete ioLayout;
    delete ioFrame;

    // delete typeBrst;
    // delete typeEye;
    // delete typePep;
    // delete typePros;
    // delete type43;
    // delete typeOther;

    delete vbox;
    delete grid;

    delete options_button;

    delete launchButton;
    delete PreviewButton;
    delete run;
    delete close;

    delete outLayout;
    delete outFrame;

    delete mainLayout;
	
    delete remainder;

    delete progress;
    delete progLabel;
    delete progLayout;
    delete progWin;
}

/*******************************************************************************
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Creating The Window~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*******************************************************************************/



/***
Function: createLayout
----------------------
Process: set-up the layout of the interface
***/
void Interface::createLayout() {
    QTextStream out(stdout);

    //Getting the alias path's
    //--------------------------
    QString EGSHOME = "$EGS_HOME";
    QString EGS_BRACHY_HOME = "$EGS_HOME/egs_brachy";
    egs_brachy_home_path = getPath(EGS_BRACHY_HOME);

    QString EGS_BRACHY_SOURCE_FOLDER = "$EGS_HOME/egs_brachy/lib/geometry/sources";
    source_folder_path = getPath(EGS_BRACHY_SOURCE_FOLDER);

    working_path = QCoreApplication::applicationDirPath();
    density = working_path + "/default_CT_calib.hu2rho"; //HU to density file

    setup_default_files();
    //Getting the current date time (used to name the egsphant, egsinp files)
    //-----------------------------------------------------------------------
    QString empty; //an empty QString
    char date[16];
    time_t curr_time;
    tm *curr_tm;

    time(&curr_time);
    curr_tm = localtime(&curr_time);

    strftime(date, 16, "%Y%m%d%H%M%S", curr_tm);
    QString date_string(date);


    //Setting up the classes
    //----------------------
    egs_input = new egsinp;
    egs_input_phantom = new egsinp;


    //Initilizing egsphant, egsinp and transformation file paths
    //-----------------------------------------------------------
    egs_input->egs_home = egs_brachy_home_path;
    egs_input->seed_file = egs_brachy_home_path + "/transformation_" + date_string;
    egs_input->egsinp_path = egs_brachy_home_path + "/egsinp_" + date_string + ".egsinp";
    egs_input->egsphant_location = egs_brachy_home_path + "/egsphant_" + date_string + ".egsphant";

    options = new Options(this, egs_brachy_home_path);
    options->setDisabled(TRUE);
    connect(options, SIGNAL(closed()), this, SLOT(closed_metrics()));

    select_location = new File_selector(this, egs_brachy_home_path, egs_input->egsinp_path,
                                        egs_input->egsphant_location, egs_input->seed_file);

    egs_input_phantom->egs_home = egs_brachy_home_path;

    //Setting the default egs_brachy files
    //------------------------------------
    egs_input->muen_file_substring = muen_file_substring;
    egs_input->low_energy_transport_file = low_energy_transport_file;
    egs_input->high_energy_transport_file = high_energy_transport_file;
    egs_input->material_data = material_data_file;
    egs_input->spectra_files = spectrum_files;

    egs_input_phantom->muen_file_substring = muen_file_substring;
    egs_input_phantom->low_energy_transport_file = low_energy_transport_file;
    egs_input_phantom->high_energy_transport_file = high_energy_transport_file;
    egs_input_phantom->material_data = material_data_file;
    egs_input_phantom->spectra_files = spectrum_files;

    //Initializing more classes
    //-------------------------
    select_location_phantom = new File_selector(this, egs_brachy_home_path, egs_input->egsinp_path, empty, empty);

    egs_from_phantom(); //Get the phantom data


//Load DICOM File Widget Setup ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    DICOMButton = new QPushButton(tr("Load a DICOM folder"));

    CalibButton = new QPushButton(tr("Load a CT calibration file"));
    DICOMButton->setToolTip(tr("This button allows the user to load a\n") +
                            tr("DICOM folder with CT, Plan and Structure \n files."));
    CalibButton->setToolTip(tr("This button allows the user to load a\n") +
                            tr("CT calibration file (HU to density). \n") +
                            tr("If no file is uploaded, a default calibration is used."));

    QPixmap image(working_path + "/egs_BRACHY_gui.jpg");
    QLabel *imageLabel = new QLabel();
    imageLabel->setPixmap(image);
	imageLabel->setAlignment(Qt::AlignCenter);

    ioLayout = new QGridLayout();
    ioLayout->addWidget(DICOMButton, 0, 0, 1, 1);
    ioLayout->addWidget(CalibButton, 1, 0, 1, 1);
    ioFrame = new QGroupBox(tr("Inputs"));
    ioFrame->setLayout(ioLayout);


//Creating the TAS Selection Pane ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    grid = new QGroupBox(tr("Select the appropriate tissue assignment scheme:"));
    vbox = new QGridLayout();
    tas_list = new QListWidget();
    tas_list->addItems(tas_names);
    tas_list->setSelectionMode(QAbstractItemView::SingleSelection);
    tas_list->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    select_tas_bttn = new QPushButton(tr("Upload a TAS"));
    tg43_simul_check = new QRadioButton(tr("Run a TG-43 simulation"));
    vbox->addWidget(tas_list,0,0);
    vbox->addWidget(select_tas_bttn,1,0);
    vbox->addWidget(tg43_simul_check,2,0);


    grid->setToolTip(tr("Selecting the tissue assignment scheme determines which\n")+
                     tr("organs and tissues are assigned in the CT data."));
    grid->setLayout(vbox);

    //typeBrst->setChecked(1);


    //Creating the advanced options button -opens window for user to make changes
    options_button = new QPushButton(tr("Advanced options"));
    options_button->setToolTip(tr("This button allows the user to \n") +
                               tr("change default values in the \n")+
                               tr("egs_brachy input files."));

    save_file_location_button = new QPushButton(tr("Change the default input file locations"));
    save_file_location_button->setToolTip(tr("This button allows the user to \n") +
                                          tr("change the default location or name\n") +
                                          tr("of the egs_brachy input files. The user \n") +
                                          tr("can enter a header for the egsinp file."));

    trim_button = new QPushButton(tr("Change egsphant boundaries or add geometries"));
    trim_button->setToolTip(tr("This button allows the user to change the egsphant boundaries, inscribe\n") +
                            tr("geometries withing the egsphant or inscribe a geometry outside the phant.\n")+
                            tr("Button is enabled after DICOM files have been parsed."));

    optionsLayout = new QGridLayout();
    optionsLayout->addWidget(options_button, 0,0,1,1);
    optionsLayout->addWidget(save_file_location_button,1,0,1,1);
    optionsLayout->addWidget(trim_button,2,0,1,1);
    optionsFrame = new QGroupBox(tr("Simulation parameters"));
    optionsFrame->setLayout(optionsLayout);


//Launch/run/peview buttons  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    launchButton = new QPushButton(tr("Launch egs_brachy"));
    PreviewButton = new QPushButton(tr("Preview the Phantom"));
    run = new QPushButton(tr("Create input files"));

    close = new QPushButton(tr("Close"));
    close->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);       //Want to be half the width

    PreviewButton->setToolTip(tr("Button is enabled after phantom has been\n") +
                              tr("created by selecting the \'Create Input Files\'\n") +
                              tr("button"));

    launchButton->setToolTip(tr("Button is enabled after phantom has been\n") +
                             tr("created by selecting the \'Create Input Files\'\n") +
                             tr("button"));

    run->setToolTip(tr("Button is enabled once DICOM files are parsed"));

    launchButton->setEnabled(0);
    trim_button->setEnabled(0);
    PreviewButton->setEnabled(0);
    run->setEnabled(0);

    outLayout = new QGridLayout();
    outLayout->addWidget(PreviewButton, 1, 0, 1, 1); //want to make span multiple columns
    outLayout->addWidget(run, 0, 0, 1, 1);
    outLayout->addWidget(launchButton, 2, 0, 1, 1);

    outFrame = new QGroupBox(tr("Execution"));
    outFrame->setLayout(outLayout);

    //------------------------------------------------
    //Creating the main tab window, adding all frames
    //------------------------------------------------
    mainLayout = new QGridLayout();

    mainLayout->addWidget(imageLabel, 0, 0, 1, 3);
    mainLayout->addWidget(ioFrame, 1, 0, 1, 1);
    mainLayout->addWidget(optionsFrame, 1, 1, 1, 1);
    mainLayout->addWidget(outFrame, 1, 2, 1, 1);
    mainLayout->addWidget(grid, 2, 0, 1, 3);
	mainLayout->setRowStretch(2,2);

    QWidget *mainPage = new QWidget();
    mainPage->setLayout(mainLayout);

    //------------------------------------
    //Creating the phantom tab
    //------------------------------------
    QWidget *phantPage = new QWidget();


    //Column 1: Pane of QListWidget's
    phant_selection = new QListWidget();
    phant_selection->addItems(phant_names);
    phant_selection->setSelectionMode(QAbstractItemView::SingleSelection);
    phant_selection->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    select_phantom = new QPushButton(tr("Upload a phantom"));
    trim_phantom_inputfiles = new QPushButton(tr("Trim the selected phantom or inscribe geometries"));
    trim_phantom_inputfiles->setDisabled(true);
    trim_phantom_inputfiles->setToolTip(tr("This button is enabled when a phant file is selected from above."));

    phantBox = new QGroupBox(tr("Select or upload the phantom"));
    QGridLayout *sbox1 = new QGridLayout();
    sbox1->addWidget(phant_selection,0,0,1,2);
    sbox1->addWidget(select_phantom,1,0,1,2);
    sbox1->addWidget(trim_phantom_inputfiles,2,0,1,2);
    phantBox->setLayout(sbox1);

    seed_selection = new QListWidget();
    seed_selection->addItems(seed_names);
    seed_selection->setSelectionMode(QAbstractItemView::SingleSelection);
    seed_selection->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    select_transformation_file = new QPushButton(tr("Upload a seed transformation file"));

    seedBox = new QGroupBox(tr("Select or upload the seed transformation file"));
    QGridLayout *sbox2 = new QGridLayout();
    sbox2->addWidget(seed_selection,0,0,1,2);
    sbox2->addWidget(select_transformation_file,1,0,1,2);
    seedBox->setLayout(sbox2);

    transport_selection = new QListWidget();
    transport_selection->addItems(transport_names);
    transport_selection->setSelectionMode(QAbstractItemView::SingleSelection);
    transport_selection->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    select_transport = new QPushButton(tr("Upload a transport parameter file"));

    transportBox = new QGroupBox(tr("Select or upload a transport parameter file"));
    QGridLayout *sbox6 = new QGridLayout();
    sbox6->addWidget(transport_selection,0,0,1,2);
    sbox6->addWidget(select_transport,1,0,1,2);
    transportBox->setLayout(sbox6);

    //Column 2: EGSINP PARAMETERS
    //---------------------------
    //selecting the run mode
    runMode = new QGroupBox(tr("Select the run mode"));
    superposition = new QRadioButton("superposition");
    normal = new QRadioButton("normal");
    volume_correction = new QRadioButton(tr("volume correction only"));
    QGridLayout *sbox3 = new QGridLayout();
    sbox3->addWidget(normal,0,0,1,2);
    sbox3->addWidget(superposition,1,0,1,2);
    sbox3->addWidget(volume_correction,2,0,1,2);
    normal->setChecked(1);
    runMode->setLayout(sbox3);

    //score energy deposition
    energyDepos = new QGroupBox(tr("Score energy deposition"));
    QGridLayout *sbox4 = new QGridLayout();
    no = new QRadioButton("No");
    yes = new QRadioButton("Yes");
    sbox4->addWidget(no, 0, 0, 1, 2);
    sbox4->addWidget(yes, 1, 0, 1, 2);
    no->setChecked(1);
    energyDepos->setLayout(sbox4);

    //number of histories
    numbHist = new QGroupBox(tr("Run parameters"));
    QGroupBox *number_histories_box = new QGroupBox(tr("Change the default number of histories"));
    numb_histories = new QLineEdit();
    numb_histories->setText("1e8");
    QGridLayout *histories_Layout = new QGridLayout();
    histories_Layout->addWidget(numb_histories, 1, 0, 1, 2);
    number_histories_box->setLayout(histories_Layout);

    QGroupBox *number_batch_box = new QGroupBox(tr("Change the default number of batches"));
    numb_batch = new QLineEdit();
    numb_batch->setText("1");
    QGridLayout *batch_Layout = new QGridLayout();
    batch_Layout->addWidget(numb_batch, 1, 0, 1, 2);
    number_batch_box->setLayout(batch_Layout);

    QGroupBox *number_chunk_box = new QGroupBox(tr("Change the default number of chunks"));
    numb_chunk = new QLineEdit();
    numb_chunk->setText("10");
    QGridLayout *chunk_Layout = new QGridLayout();
    chunk_Layout->addWidget(numb_chunk, 1, 0, 1, 2);
    number_chunk_box->setLayout(chunk_Layout);

    //change default muen file
    change_muen = new QPushButton(tr("Change the default muen file"));
    change_muen->setToolTip(tr("The default muen file is the recommended file from\n") +
                            tr("the 'README' file in the egs_brachy muen folder.   "));
    change_material_file = new QPushButton(tr("Change the default material data file"));
    change_material_file->setToolTip(tr("The default file is material.dat file \n") +
                                     tr("in egs_brachy.   "));

    //add header, change file location
    save_file_location_phantom = new QPushButton(tr("Change default ")+
            tr("file locations"));

    launch_phantom = new QPushButton(tr("Create input files and launch egs_brachy"));


    QGridLayout *sbox5 = new QGridLayout();
    sbox5->addWidget(number_histories_box, 0, 0, 1, 1);
    sbox5->addWidget(number_batch_box, 1, 0, 1, 1);
    sbox5->addWidget(number_chunk_box, 2, 0, 1, 1);

    numbHist->setLayout(sbox5);
    //numbHist->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    QGridLayout *phantpageLayout = new QGridLayout();

    phantpageLayout->addWidget(phantBox, 0, 0, 1, 1);
    phantpageLayout->addWidget(seedBox, 0, 1, 1, 1);
    phantpageLayout->addWidget(transportBox, 0, 2, 1, 1);
	
    phantpageLayout->addWidget(runMode, 1, 0, 2, 1);
    phantpageLayout->addWidget(energyDepos, 3, 0, 1, 1);
	
    phantpageLayout->addWidget(numbHist, 1, 1, 3, 1);
	
	QVBoxLayout *buttonList = new QVBoxLayout();
	
    buttonList->addWidget(change_muen);
    buttonList->addWidget(change_material_file);
    buttonList->addWidget(save_file_location_phantom);
    buttonList->addStretch();
    buttonList->addWidget(launch_phantom);
    phantpageLayout->addLayout(buttonList, 1, 2, 3, 1);

	phantpageLayout->setRowStretch(0,2);
    phantPage->setLayout(phantpageLayout);


    //------------------------------------
    //Creating the additional functions tab
    //------------------------------------
    QWidget *additionalPage = new QWidget();


    //Modular Pane ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    CTButton = new QPushButton(tr("Load DICOM file(s)"));
    CTButton->setToolTip(tr("This button allows the user to load \n") +
                         tr("DICOM CT or plan file to create egsinp\n") +
                         tr("file(s) relevant to the type of DICOM file."));

    calibrationButton = new QPushButton(tr("Load a CT calibration file"));
    calibrationButton-> setToolTip(tr("This button is enabled if a DICOM CT \n file is uploaded\n"));
    calibrationButton->setDisabled(true);

    //tissue assignment scheme
    // typeBrst1 = new QRadioButton("Breast");
    // typeEye1 = new QRadioButton("Eye Plaque");
    // typePep1 = new QRadioButton("Peppa Breast");
    // typePros1 = new QRadioButton("Prostate");
    type431 = new QRadioButton("TG-43");
    // typeOther1 = new QRadioButton("Other");


    tas_grid = new QGroupBox(tr("Select the appropriate tissue assignment scheme:"));
    tas_grid->setToolTip(tr("If DICOM CT and Structure files are loaded, this assignment\n")+
                         tr("scheme pane will become enabled. The scheme is used\n") +
                         tr("to identify the organs and tissues in the data."));

    QGridLayout *vbox1 = new QGridLayout();
    tas_selection = new QListWidget();
    tas_selection->addItems(tas_names);
    tas_selection->setSelectionMode(QAbstractItemView::SingleSelection);
    tas_selection->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    select_tas = new QPushButton(tr("Upload the TAS"));
    tg43_simul = new QRadioButton(tr("Run a TG-43 simulation"));
    vbox1->addWidget(tas_selection,0,0);
    vbox1->addWidget(select_tas,1,0);
    vbox1->addWidget(tg43_simul,2,0);

    tas_grid->setLayout(vbox1);


    tas_grid->setDisabled(true);

    //selecting the phantom
    phant_selection_inputfiles = new QListWidget();
    phant_selection_inputfiles->addItems(phant_names);
    phant_selection_inputfiles->setSelectionMode(QAbstractItemView::SingleSelection);
    phant_selection_inputfiles->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    select_phantom_inputfiles = new QPushButton(tr("Upload the phantom"));

    phantBox_inputfiles = new QGroupBox(tr("Select or upload the phantom"));
    QVBoxLayout *sbox7 = new QVBoxLayout();
    sbox7->addWidget(phant_selection_inputfiles,0,0);
    sbox7->addWidget(select_phantom_inputfiles,1,0);
    phantBox_inputfiles->setLayout(sbox7);

    phantBox_inputfiles->setDisabled(true);
    phantBox_inputfiles->setToolTip(tr("If a DICOM Plan file is loaded and DICOM CT \n") +
                                    tr("data isn't, the phantom pane will become enabled.\n")+
                                    tr("The selection is used to create the \n") +
                                    tr("egsinp file."));

    //selecting the seed transformation
    seed_selection_inputfiles = new QListWidget();
    seed_selection_inputfiles->addItems(seed_names);
    seed_selection_inputfiles->setSelectionMode(QAbstractItemView::SingleSelection);
    seed_selection_inputfiles->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    select_seed_inputfiles = new QPushButton(tr("Upload the seed transformation file"));


    seedBox_inputfiles = new QGroupBox(tr("Select or upload the seed transformation"));
    QVBoxLayout *sboxs = new QVBoxLayout();
    sboxs->addWidget(seed_selection_inputfiles,0,0);
    sboxs->addWidget(select_seed_inputfiles,1,0);
    seedBox_inputfiles->setLayout(sboxs);

    seedBox_inputfiles->setDisabled(true);
    seedBox_inputfiles->setToolTip(tr("If a DICOM CT files are loaded and a DICOM Plan file\n") +
                                   tr("isn't, the seed transformation pane will become enabled.\n")+
                                   tr("The selection is used to create the \n") +
                                   tr("egsinp file."));

    create_input_files_button = new QPushButton(tr("Create the relevant input file(s)"));
    create_input_files_button->setToolTip(tr("Creates the relevant egs_brachy input files\n")+
                                          tr("based on the type of DICOM file(s) loaded."));
    create_input_files_button->setDisabled(true);
    //dose_metrics = new QPushButton(tr("Calculate metrics for dose data"));

    three_ddose_to_dose = new QPushButton(tr("3ddose to DICOM dose file"));
    three_ddose_to_dose->setToolTip(tr("This button allows the user to load a\n") +
                                    tr(".3ddose file to create a DICOM Dose \n") +
                                    tr("file."));
    create_dose_to_3ddose = new QPushButton(tr("DICOM dose to 3ddose file"));
    create_dose_to_3ddose->setToolTip(tr("This button allows the user to load a\n") +
                                      tr("DICOM dose file to create a .3ddose \n") +
                                      tr("file."));

    compare_3ddose = new QPushButton(tr("Compare 3ddose"));

    CTButton->resize(CTButton->minimumSize());
    three_ddose_to_dose->resize(three_ddose_to_dose->minimumSize());

    fromdicomLayout = new QGridLayout();
    fromdicomLayout->addWidget(CTButton, 1, 0, 1, 1);
    fromdicomLayout->addWidget(calibrationButton, 2, 0, 1, 1);
    fromdicomLayout->addWidget(tas_grid, 3, 0, 4, 1);
    fromdicomLayout->addWidget(phantBox_inputfiles, 3,1,1,2);
    fromdicomLayout->addWidget(seedBox_inputfiles, 4,1,1,2);
    fromdicomLayout->addWidget(create_input_files_button, 5, 1, 1, 1);
    fromdicom = new QGroupBox(tr("Create egs_brachy input file(s) from DICOM data"));
    fromdicom->setLayout(fromdicomLayout);

    from3ddoseLayout = new QGridLayout();
    from3ddoseLayout->addWidget(three_ddose_to_dose, 1, 0, 1, 1);
    from3ddoseLayout->addWidget(create_dose_to_3ddose, 2, 0, 1, 1);
    //from3ddoseLayout->addWidget(compare_3ddose, 3, 0, 1, 1);
    from3ddose = new QGroupBox(tr("Convert dose files"));
    from3ddose->setLayout(from3ddoseLayout);

    modularLayout = new QGridLayout();
    modularLayout->addWidget(fromdicom,0,0,1,3);
    modularLayout->addWidget(from3ddose,1,0,1,1);

    additionalPage->setLayout(modularLayout);

    //-------------------------------
    // Setting up the tab widget
    //-------------------------------
    QTabWidget *widget_tabs = new QTabWidget;

    widget_tabs->addTab(mainPage, tr("Using DICOM Data"));
    widget_tabs->addTab(phantPage, tr("Using the egs_brachy Library"));
    widget_tabs->addTab(additionalPage, tr("Extra options"));
	
    //QScrollArea *scrollArea = new QScrollArea();
    //scrollArea->setWidget(widget_tabs);
    //scrollArea->viewport()->setAutoFillBackground(true);
    //scrollArea->setWindowTitle(QObject::tr(""));
    //scrollArea->show();

    QGridLayout *Layout = new QGridLayout;
    Layout->addWidget(widget_tabs,0,0,1,1);
    Layout->addWidget(close,1,0,1,1, Qt::AlignRight);
	Layout->setRowStretch(0,5);
    setLayout(Layout);
    resize(minimumSize());
    //resize(1280,720);        //resizing the main window
    setWindowTitle(tr("egs_brachy_GUI"));


// Progress Bar ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    remainder = new double (0.0);
    progWin = new QWidget();
    progLayout = new QGridLayout();
    progress = new QProgressBar();
    progLabel = new QLabel();
    progLayout->addWidget(progress, 0, 0);
    progWin->setLayout(progLayout);
    progWin->resize(300, 0);
    progress->setRange(0, 1000000000);

}

/***
Function: connectLayout
-----------------------
Process: conects the Qt Widget's to functions (signals and slots)
***/
void Interface::connectLayout() {
    connect(close, SIGNAL(clicked()),
            this, SLOT(close()));

    connect(close, SIGNAL(clicked()),
            qApp, SLOT(quit()));

    connect(DICOMButton, SIGNAL(clicked()),
            this, SLOT(load_dicom()));

    connect(CalibButton, SIGNAL(clicked()),
            this, SLOT(loadCalib()));

    connect(calibrationButton, SIGNAL(clicked()),
            this, SLOT(loadCalib()));

    connect(run, SIGNAL(clicked()),
            this, SLOT(check_dicom_inputs()));

    connect(tg43_simul, SIGNAL(released()),
            this, SLOT(checkTG43()));

    connect(tg43_simul_check, SIGNAL(released()),
            this, SLOT(checkTG43simul()));

    connect(options_button, SIGNAL(clicked()),
            this, SLOT(show_advanced_options()));

    connect(trim_button, SIGNAL(clicked()),
            this, SLOT(show_trim_options()));

    connect(save_file_location_button, SIGNAL(clicked()),
            this, SLOT(change_file_location()));

    connect(launchButton, SIGNAL(clicked()),
            this, SLOT(show_launch_egs()));

    connect(select_tas_bttn, SIGNAL(clicked()),
            this, SLOT(select_tas_file()));

    connect(select_tas, SIGNAL(clicked()),
            this, SLOT(select_tas_file()));

    connect(select_phantom, SIGNAL(clicked()),
            this, SLOT(select_phantom_file()));

    connect(phant_selection, SIGNAL(currentRowChanged(int)),
            this, SLOT(enable_midTab_trim(int)));

    connect(trim_phantom_inputfiles, SIGNAL(clicked()),
            this, SLOT(showTrim_midtab()));

    connect(select_phantom_inputfiles, SIGNAL(clicked()),
            this, SLOT(select_phantom_file()));

    connect(select_transformation_file, SIGNAL(clicked()),
            this, SLOT(select_transform_file()));

    connect(select_seed_inputfiles, SIGNAL(clicked()),
            this, SLOT(select_transform_file()));

    connect(select_transport, SIGNAL(clicked()),
            this, SLOT(select_transport_file()));

    connect(launch_phantom, SIGNAL(clicked()),
            this, SLOT(run_brachy_phantom()));

    connect(change_muen, SIGNAL(clicked()),
            this, SLOT(load_muen()));

    connect(change_material_file, SIGNAL(clicked()),
            this, SLOT(load_material()));

    connect(save_file_location_phantom, SIGNAL(clicked()),
            this, SLOT(change_phantom_file_location()));

    connect(CTButton, SIGNAL(clicked()),
            this, SLOT(loadDICOMfile()));

    connect(create_input_files_button, SIGNAL(clicked()),
            this, SLOT(setup_input_files()));

    connect(three_ddose_to_dose, SIGNAL(clicked()),
            this, SLOT(read_3ddose()));

    connect(create_dose_to_3ddose, SIGNAL(clicked()),
            this, SLOT(launch_dose_to_3ddose()));

    connect(PreviewButton, SIGNAL(clicked()),
            this, SLOT(show_preview()));


}



/***
Function: show_launch_egs
-------------------------------
Process: launches egs_brachy and shows console pop-up
         Code was adapted from Martin Martinov brachydose_gui
***/
void Interface::show_launch_egs() {
    this->setDisabled(true);

    QStringList options = {"Interactive", "Batch"};
    bool ok;
    QString item = QInputDialog::getItem(this, tr("Select the egs_brachy run mode"),
                                         tr("Select the egs_brachy run mode:                   "),
                                         options, 0, false, &ok);
    if (ok && !item.isEmpty()) {
        QTextStream out(stdout);
        out<<"Running egs_brachy in console" <<endl;

        //-----------------------------------------
        // From martin
        //------------------------------------------
        // Console
        console = new QTextEdit();
        console->setReadOnly(TRUE);
        close1 = new QPushButton(tr("Close"));
        kill = new QPushButton(tr("Kill"));
        kill->setEnabled(0);
        metrics_button = new QPushButton(tr("View metrics and output DICOM dose file"));
        metrics_button->setDisabled(true);
        conLayout = new QGridLayout();
        conLayout->addWidget(console, 0, 0, 1, 3);
        conLayout->addWidget(kill, 1, 0);
        conLayout->addWidget(close1, 1, 1);
        conLayout->addWidget(metrics_button, 1, 2);
        conWin = new QWidget();
        conWin->setHidden(TRUE);
        conWin->resize(775, 400);
        conWin->setWindowTitle(tr("Console"));
        conWin->setLayout(conLayout);


        conWin->show();
        conWin->activateWindow();
        conWin->raise();

        connect(close1, SIGNAL(clicked()),
                this, SLOT(closeWindow()));


        //command for running egs_brachy
        QString command;
        if (item == "Interactive") {
            command = "egs_brachy -i " + egs_input->egsinp_name;
        }
        else if (item == "Batch") {
            command = "exb egs_brachy " + egs_input->egsinp_name + " pegsless";
        }
        else {
            return;
        }

        std::cout<<command.toStdString() <<"\n";
        read_error = false;

        // This empties the console and readies it for use
        console->setText("");
        conWin->show();
        conWin->activateWindow();
        conWin->raise();


        // Create the process that will run egs_brachy in the background,and connect
        // it to all the functions implemented for processes
        QProcess *brachy = new QProcess();

        brachy->setProcessChannelMode(QProcess::MergedChannels);

        connect(kill, SIGNAL(clicked()),
                brachy, SLOT(kill()));

        connect(brachy, SIGNAL(readyReadStandardOutput()),
                this, SLOT(readOutput()));

        connect(brachy, SIGNAL(readyReadStandardError()),
                this, SLOT(readError()));

        connect(brachy, SIGNAL(finished(int, QProcess::ExitStatus)),
                this, SLOT(reenableButtons()));

        connect(metrics_button, SIGNAL(clicked()),
                this, SLOT(read_3ddose_output_metrics()));

        brachy->setReadChannel(QProcess::StandardOutput);

        // Actual line of exectuion
        brachy->start(command);

        conWin->setWindowTitle(tr("Console (Running egs_brachy)"));
        kill->setEnabled(1);
    }
    this->setEnabled(true);
}


/***
Function: show_launch_egs
-------------------------------
Process: launches egs_brachy and shows console pop-up
         Code was adapted from Martin Martinov brachydose_gui
***/
void Interface::show_launch_egs_phantom() {
    this->setDisabled(true);
    QTextStream out(stdout);
    out<<"Running egs_brachy in console" <<endl;

    //-----------------------------------------
    // From martin
    //------------------------------------------

    // Console
    console = new QTextEdit();
    console->setReadOnly(TRUE);
    close1 = new QPushButton(tr("Close"));
    kill = new QPushButton(tr("Kill"));
    kill->setEnabled(0);
    metrics_button = new QPushButton(tr("View metrics and output DICOM dose file"));
    metrics_button->setDisabled(true);
    conLayout = new QGridLayout();
    conLayout->addWidget(console, 0, 0, 1, 3);
    conLayout->addWidget(kill, 1, 0);
    conLayout->addWidget(close1, 1, 1);
    conLayout->addWidget(metrics_button, 1, 2);
    conWin = new QWidget();
    conWin->setHidden(TRUE);
    conWin->resize(775, 400);
    conWin->setWindowTitle(tr("Console"));
    conWin->setLayout(conLayout);


    conWin->show();
    conWin->activateWindow();
    conWin->raise();

    connect(close1, SIGNAL(clicked()),
            this, SLOT(closeWindow()));


    //command for running egs_brachy
    QString command = "egs_brachy -i " + egs_input_phantom->egsinp_name;
    std::cout<<command.toStdString() <<"\n";
    read_error = false;

    // This empties the console and readies it for use
    console->setText("");
    conWin->show();
    conWin->activateWindow();
    conWin->raise();


    // Create the process that will run egs_brachy in the background,and connect
    // it to all the functions implemented for processes
    QProcess *brachy = new QProcess();

    brachy->setProcessChannelMode(QProcess::MergedChannels);

    connect(kill, SIGNAL(clicked()),
            brachy, SLOT(kill()));

    connect(brachy, SIGNAL(readyReadStandardOutput()),
            this, SLOT(readOutput()));

    connect(brachy, SIGNAL(readyReadStandardError()),
            this, SLOT(readError()));

    connect(brachy, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(reenableButtons_phantom()));

    connect(metrics_button, SIGNAL(clicked()),
            this, SLOT(read_3ddose_output_metrics()));

    brachy->setReadChannel(QProcess::StandardOutput);

    // Actual line of exectuion
    brachy->start(command);

    conWin->setWindowTitle(tr("Console (Running egs_brachy)"));
    kill->setEnabled(1);


}

void Interface::readOutput() { // When a process gives the
    // readyReadStandardOutput() signals, this
    // function puts the output into the console
    QProcess *proc = (QProcess *)sender();
    QString temp = proc->readAllStandardOutput();

    console->append(temp);
    console->repaint();
}


void Interface::readError() { // When a process gives the
    // readyReadErrorOutput() signals, this
    // function puts the output into the console
    QProcess *proc = (QProcess *)sender();
    QString temp = proc->readAllStandardError();

    console->append(temp);
    console->repaint();

    read_error = true;
}


void Interface::reenableButtons() { // Allow the user to run egs_brachy again
    // when egs_brachy is finished executing
    QTextStream out(stdout);
    out<<"Finished running egs_brachy";
    console->append("\n\n\n\nFinished running egs_brachy");

    QProcess *proc = (QProcess *)sender();
    proc->terminate();
    delete proc;

    conWin->setWindowTitle(tr("Console"));
    kill->setDisabled(true);

    //Sucessfully run if the phantom path exists
    QFileInfo phantom_path = QFileInfo(egs_brachy_home_path + "/" + egs_input->egsinp_name + ".phantom.3ddose");
    QFileInfo mederr = QFileInfo(egs_brachy_home_path + "/" + egs_input->egsinp_name + ".mederr");

    if (phantom_path.exists() || mederr.exists()) {
        if (phantom_path.exists()) {
            metrics_button->setEnabled(true);
        }
        out<<" successfully" <<endl;
        console->append(" successfully");
    }
    else {
        out<<" unsuccessfully" <<endl;
        console->append(" unsuccessfully");
    }


}


void Interface::closeWindow() {
    // This closes the Run egs_brachy, Settings, Preferences, and Console wins
    conWin->hide();
    this->setEnabled(true);

}

/**
Function: reenableButtons_phantom
--------------
Process: re-enables the console window buttons
***/
void Interface::reenableButtons_phantom() { // Allow the user to run egs_brachy again
    // when egs_brachy is finished executing
    QTextStream out(stdout);
    out<<"Finished running egs_brachy";
    console->append("\n\n\n\nFinished running egs_brachy");

    QProcess *proc = (QProcess *)sender();
    proc->terminate();
    delete proc;

    conWin->setWindowTitle(tr("Console"));
    kill->setDisabled(true);

    //Sucessfully run if the phantom path exists
    QFileInfo phantom_path = QFileInfo(egs_brachy_home_path + "/" + egs_input_phantom->egsinp_name + ".phantom.3ddose");
    QFileInfo mederr = QFileInfo(egs_brachy_home_path + "/" + egs_input_phantom->egsinp_name + ".mederr");

    if (phantom_path.exists() || mederr.exists()) {
        if (phantom_path.exists()) {
            metrics_button->setEnabled(true);
        }
        out<<" successfully" <<endl;
        console->append(" successfully");
    }
    else {
        out<<" unsuccessfully" <<endl;
        console->append(" unsuccessfully");
    }


}


/***
Function: read_3ddose_output_metrics
------------------------------------
Process: Reads the 3ddose file created by egs_brachy and creates the DICOM dose file
        Makes sure the 3ddose file exists and is readable. Checks the dicom path is writeable
        Calculates the metrics and opens metrics window

***/
void Interface::read_3ddose_output_metrics() {
    conWin->hide();
    this->setDisabled(true);
    QTextStream out(stdout);

    //read the 3ddose file
    QString path = egs_brachy_home_path + "/" + egs_input->egsinp_name + ".phantom.3ddose";

    QFileInfo file_3ddose(path);
    if (!file_3ddose.exists()) {
        out<<"Unable to create metrics. The 3ddose file (" <<path <<") does not exist \n";

    }
    else  if (!file_3ddose.isReadable()) {
        out<<"Unable to read the 3ddose file " <<path <<"\n";

    }
    else {

        QString path2 = QFileDialog::getSaveFileName(0, "Save the DICOM Dose file",
                        egs_brachy_home_path, "DICOM (*.dcm)");

        if (!path2.isEmpty()) {

            QFileInfo file_dicomdose(path2);

            dose = new read_dose;
            dose->load_dose_data(path);

            //format and output the DICOM dose file
            dose->create_dicom_dose(get_data->dicomHeader, path2);

            calc_metrics = new metrics;

            QMap <int, QString> names;
            for (int i=0; i<structUnique.size(); i++)
                if (structUnique[i] && !external[i]) {
                    names.insert(i, get_data->structName[i]);
                }

            calc_metrics->get_data(dose->x, dose->y, dose->z, dose->cx, dose->cy, dose->cz, phant.contour,
                                   dose->val, dose->err, names);

            connect(calc_metrics, SIGNAL(closed()), this, SLOT(closed_metrics()));


            calc_metrics->window->show();
            QApplication::processEvents();
            calc_metrics->setEnabled(TRUE);
        }
    }
    this->setEnabled(true);
}


/***
Function: closed_metrics
------------------------
Process: re-enables the main widget when the window pane is closed
***/
void Interface::closed_metrics() {
    this->setEnabled(true);
    QApplication::processEvents();
}

/*********************************************************************************
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Create DICOM Dose from Phantom~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**********************************************************************************/

/***
Function: egs_from_phantom
--------------------------
Process: Gets a list of all the phantoms, transformation file and transport parameters
         in egs_brachy

***/
void Interface::egs_from_phantom() {
    //extract list of tas
    //----------------------------------------------------------------
    QString tas_path = working_path + "/Tissue_Assignment_Schemes/";
    QDir dir_tas(tas_path);
    QFileInfoList fileName_tas = dir_tas.entryInfoList();


    for (int i=0; i < fileName_tas.size(); i ++) {
        QFileInfo fileInfo = fileName_tas.at(i);
        if (fileInfo.isFile()) {
            tas_names.append(fileInfo.completeBaseName());
            tas_names_path.append(fileInfo.absoluteFilePath());
        }
    }
    //extract list of phantoms from the egs_brachy library
    //----------------------------------------------------------------
    QString phantoms_path = egs_brachy_home_path + "/lib/geometry/phantoms/";
    QDir dir(phantoms_path);
    QFileInfoList fileName = dir.entryInfoList();


    for (int i=0; i < fileName.size(); i ++) {
        QFileInfo fileInfo = fileName.at(i);
        if (fileInfo.isFile()) {
            phant_names.append(fileInfo.completeBaseName());
            phant_names_path.append(fileInfo.absoluteFilePath());
        }
    }

    //extract list of seed transformation files
    //----------------------------------------------------------
    QString seed_path = egs_brachy_home_path + "/lib/geometry/transformations/";
    QDir dir_seed(seed_path);
    QFileInfoList fileName_seed = dir_seed.entryInfoList();

    for (int i=0; i < fileName_seed.size(); i ++) {
        QFileInfo fileInfo_seed = fileName_seed.at(i);
        if (fileInfo_seed.isFile()) {
            seed_names.append(fileInfo_seed.fileName());
            seed_names_path.append(fileInfo_seed.absoluteFilePath());
        }
    }

    //get the list of transport parameter files
    //-----------------------------------------------------
    QString transport_path = egs_brachy_home_path + "/lib/transport/";
    QDir dir_transp(transport_path);
    QFileInfoList fileName_transp = dir_transp.entryInfoList();

    for (int i=0; i < fileName_transp.size(); i ++) {
        QFileInfo fileInfo_transp = fileName_transp.at(i);
        if (fileInfo_transp.isFile()&& !fileInfo_transp.fileName().contains("README")) {
            transport_names.append(fileInfo_transp.fileName());
            transport_names_path.append(fileInfo_transp.absoluteFilePath());
        }
    }
}

/***
Function: getPath
-----------------
Process: Returns the path of the alias
Input: path: the alias
Output: a string with the absolute path of the alias

Function obtain from Martin Martinov
***/
QString Interface::getPath(QString path) {
    QProcessEnvironment env = env.systemEnvironment();
    // If we have no environments
    if (path.at(0) != '$') {
        return path;    // Change nothing
    }

    if (path.count('/') == 0) { // If someone wants just the env variable
        return env.value(path.section('/',0,0).mid(1));
    }

    // Otherwise change the variable with its value and return the path
    return env.value(path.section('/',0,0).mid(1)) +
           path.mid(path.indexOf('/') + 1);
}


/***
Function: setup_default_files
------------------------------
Process: Reads the default_files.txt file to get the default
            transport parameter, muen and nuendat file
        Reads the file twice; first time is to ensure it has more than 5 lines
        and then to actually read each line

Format of the default_text file
    muendat file
    low energy transport parameter file
    high energy transport parameter file
**/
void Interface::setup_default_files() {
    // QString filepath = working_path + "/default_files.txt";
    // QString spectrum;
    // QFile *default_files;
    // QTextStream *input;

    // while(!checkDefaultFile(filepath)){
    // filepath = QFileDialog::getOpenFileName(
    // this,
    // tr("Select a valid default file"),
    // working_path,
    // "Text (*.txt)");
    // }

    // default_files = new QFile (filepath);
    // input = new QTextStream (default_files);

    // *input >> material_data_file;
    // *input >> muen_file_substring;
    // *input >> low_energy_transport_file;
    // *input >> high_energy_transport_file;

    // while(!input->atEnd()){ //the remaining lines are spectrun files, save them to a file
    // *input >> spectrum;
    // spectrum_files.push_back(spectrum);
    // }
    QString spectrum;
    QFile *default_files;
    QTextStream *input;
    default_files = new QFile(working_path + "/default_files.txt");

    if (!default_files->open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cout<<"Unable to open the default_files.txt which provides the location of the default egsinp files" <<std::endl;
    }
    else {
        input = new QTextStream(default_files);
        *input >> material_data_file;
        *input >> muen_file_substring;
        *input >> low_energy_transport_file;
        *input >> high_energy_transport_file;

        while (!input->atEnd()) { //the remaining lines are spectrun files, save them to a file
            *input >> spectrum;
            spectrum_files.push_back(spectrum);
        }
    }
}

bool Interface::checkDefaultFile(QString filepath) {

    QTextStream *input1;
    QFile *default_files = new QFile(filepath);

    //Check the file can be opened
    if (!default_files->open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cout<<"Unable to open " <<filepath.toStdString() <<" which provides the location of the default egsinp files" <<std::endl;
        QMessageBox msgBox;
        msgBox.setText(tr("default_files.txt could not be found in the working directory. Please select the location of the default_files (.txt) file. \nThe user guide contains a description of its contents."));
        msgBox.setWindowTitle(tr("egs_brachy GUI"));
        msgBox.exec();

        return false;
    }

    //Check the file contains at least 5 lines
    input1 = new QTextStream(default_files);
    int numlines = 0;
    while (!input1->atEnd()) {
        QString line = input1->readLine();
        numlines++;
    }
    if (numlines < 5) {
        std::cout<<"The default file " <<filepath.toStdString() <<" does not comply to specificions. Please consult the uder manual for a description of its contsnts and select a valid file. " <<std::endl;
        QMessageBox msgBox;
        msgBox.setText(tr("The contents of default_file does not comply to specifications. Please consult the user guide for a description of its contents."));
        msgBox.setWindowTitle(tr("egs_brachy GUI"));
        msgBox.exec();

        return false;
    }

    //If it meets checks, okay to parse and save contents
    return true;
}


void Interface::load_dicom() {

    PreviewButton->setDisabled(true);
    launchButton->setDisabled(true);
    run->setDisabled(true);
    this->setDisabled(true);

    QString dir_path= QFileDialog::getExistingDirectory(
                          this,
                          tr("Select a DICOM directory to open"),
                          "/data/data060/sjarvis/Dicom_data/");


    QVector <QString> tempS2;
    QDir dir(dir_path);
    QFileInfoList fileNamesRT = dir.entryInfoList();
    QDirIterator directories(dir_path, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);


    //Reads all the files in sub-directories of the dir_path
    while (directories.hasNext()) { //one more entry in the directory
        QFile f(directories.next());    //advances iterator to next entry
        QDir dir1(f.fileName());
        QFileInfoList fileNam = dir1.entryInfoList();   //all files and directories in the sub-directory

        for (int i=0; i < fileNam.size(); i ++) { //itterating through all the files
            //in the sub-directory
            QFileInfo fileInf = fileNam.at(i);

            if (fileInf.isFile()) { //Adds the file to the list of file names
                fileNamesRT.push_back(fileInf);
            }

        }

    }

    //reading each DICOM file and extracting all relevant information
    if (!fileNamesRT.isEmpty() && !dir_path.isEmpty()) {

        for (int i=0; i < fileNamesRT.size(); i ++) {

            QFileInfo fileInfo = fileNamesRT.at(i);

            if (fileInfo.isFile()) {
                tempS2.append(fileInfo.absoluteFilePath());
            }

        }


        //Parse the dicom data files
        this->setDisabled(true);

        get_data = new DICOM;
        get_data ->extract(tempS2);

        //-------------------------
        //Initialize the phantom -if all data has been loaded
        //-------------------------
        if (get_data->loadedct) {
            // Assume first slice matches the rest and set x, y, and z boundaries
            phant.nx = get_data->xPix[0];
            phant.ny = get_data->yPix[0];
            phant.nz = get_data->numZ;
            phant.x.fill(0,phant.nx+1);
            phant.y.fill(0,phant.ny+1);
            phant.z.fill(0,phant.nz+1);

            {
                QVector <char> mz(phant.nz, 0);
                QVector <QVector <char> > my(phant.ny, mz);
                QVector <QVector <QVector <char> > > mx(phant.nx, my);
                phant.m = mx;
                QVector <double> dz(phant.nz, 0);
                QVector <QVector <double> > dy(phant.ny, dz);
                QVector <QVector <QVector <double> > > dx(phant.nx, dy);
                phant.d = dx;
                QVector <int> sz(phant.nz, 0);
                QVector <QVector <int> > sy(phant.ny, sz);
                QVector <QVector <QVector <int> > > sx(phant.nx, sy);
                phant.contour = sx;
            }

            // Define xy bound values, still assuming first slice matches the rest
            for (int i = 0; i <= phant.nx; i++) {
                phant.x[i] = (get_data->imagePos[0][0]+(i-0.5)*get_data->xySpacing[0][0])/10.0;
            }

            for (int i = 0; i <= phant.ny; i++) {
                phant.y[i] = (get_data->imagePos[0][1]+(i-0.5)*get_data->xySpacing[0][1])/10.0;
            }


            // Define z bound values
            double prevZ, nextZ;
            nextZ = get_data->imagePos[0][2]-get_data->zSpacing[0]/2.0;
            for (int i = 0; i < phant.nz; i++) {
                prevZ = nextZ/2.0 + (get_data->imagePos[i][2]-get_data->zSpacing[i]/2.0)/2.0;
                nextZ = get_data->imagePos[i][2]+get_data->zSpacing[i]/2.0;
                phant.z[i] = prevZ/10.0;
            }
            phant.z.last() = nextZ/10.0;

            QVector <double> seed_bounds; //Seed bounds: xmin, xmax, ymin, ymax, zmin, zmax

            if (get_data->all_seed_pos.size() > 0) {
                seed_bounds.resize(6);
                double xmin = get_data->all_seed_pos[0][0].x, xmax = get_data->all_seed_pos[0][0].x;
                double ymin = get_data->all_seed_pos[0][0].y, ymax = get_data->all_seed_pos[0][0].y;
                double zmin = get_data->all_seed_pos[0][0].z, zmax = get_data->all_seed_pos[0][0].z;

                for (int i=0; i<get_data->all_seed_pos.size(); i++)
                    for (int j=0; j<get_data->all_seed_pos[i].size(); j++) {
                        if (get_data->all_seed_pos[i][j].x  < xmin) {
                            xmin = get_data->all_seed_pos[i][j].x;
                        }
                        if (get_data->all_seed_pos[i][j].x  > xmax) {
                            xmax = get_data->all_seed_pos[i][j].x;
                        }
                        if (get_data->all_seed_pos[i][j].y  < ymin) {
                            ymin = get_data->all_seed_pos[i][j].y;
                        }
                        if (get_data->all_seed_pos[i][j].y  > ymax) {
                            ymax = get_data->all_seed_pos[i][j].y;
                        }
                        if (get_data->all_seed_pos[i][j].z  < zmin) {
                            zmin = get_data->all_seed_pos[i][j].z;
                        }
                        if (get_data->all_seed_pos[i][j].z  > zmax) {
                            zmax = get_data->all_seed_pos[i][j].z;
                        }
                    }
                seed_bounds[0] = xmin;
                seed_bounds[1] = xmax;
                seed_bounds[2] = ymin;
                seed_bounds[3] = ymax;
                seed_bounds[4] = zmin;
                seed_bounds[5] = zmax;
            }

            trimEGS = new trim(this, get_data->structName, get_data->structType, phant.x, phant.y, phant.z, get_data->extrema, seed_bounds);
            trimEGS->egs_path = egs_brachy_home_path;

            //Enables the 'create input files' button
            run->setEnabled(1);
            trim_button->setEnabled(1);
            run ->setToolTip(tr("Creates the patient model and all input\n")+
                             tr("files required by egs_brachy"));
            trim_button->setToolTip(tr(" Trim the bounds of the egsphant, inscribe the egsphant \n within a larger region or inscribe geometries within the phantom."));
        }

    }
    this->setEnabled(true);

}


void DICOM::extract(QVector<QString> tempS2) {
    create_progress_bar_dicom();
    setup_progress_bar_dicom("Parsing DICOM Files", "");

    std::clock_t start;
    double duration;
    start = std::clock();

    // ---------------------------------------------------------- //
    // PARSE INPUT AND PUT INTO THE CORRECT VECTOR                //
    // ---------------------------------------------------------- //

    double increment = 1000000000/(2*tempS2.size());
    updateProgress(increment);


    database dat;
    QVector <DICOM *> dicom;
    QVector <DICOM *> dicomExtra;
    DICOM *dicomPlan = NULL;
    DICOM *dicomStruct = NULL;

    for (int i = 0; i < tempS2.size(); i++) {
        QString path(tempS2[i]);

        DICOM *d = new DICOM(&dat);
        if (d->parse(path)) {
            std::cout << std::dec << "Successfully parsed " << path.toStdString() << ".\n";
            dicomExtra.append(d);
        }
        else {
            std::cout << "Unsuccessfully parsed " << path.toStdString() << "\n";
        }

        updateProgress(increment);
    }


    duration = (std::clock()-start)/(double)CLOCKS_PER_SEC;
    std::cout << "Parsed the " << dicomExtra.size() << " DICOM files.  Time elapsed is " << duration << " s.\n";



    // ---------------------------------------------------------- //
    // INITIAL DICOM INFORMATION FETCHING                         //
    // ---------------------------------------------------------- //
    bool ctFlag = false;
    bool rsFlag = false;
    bool rpFlag = false;
    int count_rs = 0;
    int count_rp = 0;

    if (dicomExtra.size() > 0) {
        // Put not-CT dicom in seperate array
        for (int i = 0; i < dicomExtra.size(); i++) {
            ctFlag = false;
            rsFlag = false;
            rpFlag = false;
            for (int j = 0; j < dicomExtra[i]->data.size(); j++) {
                if (dicomExtra[i]->data[j]->tag[0] == 0x0008 && dicomExtra[i]->data[j]->tag[1] == 0x0060) {
                    QString temp = "";
                    for (unsigned int s = 0; s < dicomExtra[i]->data[j]->vl; s++) {
                        temp.append(dicomExtra[i]->data[j]->vf[s]);
                    }

                    temp = temp.trimmed();
                    if (temp =="CT") {
                        ctFlag = true;
                        loadedct = true;
                    }
                    if (temp == "RTSTRUCT") {
                        loadedstruct = true;
                        rsFlag = true;
                    }
                    if (temp == "RTPLAN") {
                        loadedplan = true;
                        rpFlag = true;
                    }
                }
            }

            if (ctFlag) {
                dicom.append(dicomExtra[i]);
                dicomExtra.remove(i--);
            }
            if (rpFlag) {
                dicomPlan = dicomExtra[i];
                dicomExtra.remove(i--);
                count_rp ++;
            }
            if (rsFlag) {
                dicomStruct = dicomExtra[i];
                dicomExtra.remove(i--);
                count_rs ++;
            }
            updateProgress(increment);
        }

        if (count_rs > 1) {
            std::cout<<"Warning: Parsed multiple DICOM struct files." <<count_rs << "\n" ;
        }

        if (count_rp > 1) {
            std::cout<<"Warning: Parsed multiple DICOM plan files." <<count_rp <<" \n" ;
        }



        progress2->setValue(1000000000);


        // ---------------------------------------------------------- //
        // FETCH CT DATA FROM DICOM FILES                             //
        // ---------------------------------------------------------- //
        increment = 1000000000/(2 + dicom.size());
        setup_progress_bar_dicom("Extracting Data From Files", "");

        if (loadedct) {

            // Sort all CT slices by z height
            mergeSort(dicom,dicom.size());

            duration = (std::clock()-start)/(double)CLOCKS_PER_SEC;
            std::cout << "Sorted the " << dicom.size() << " DICOM CT files along z.  Time elapsed is " << duration << " s.\n";

            numZ = dicom.size(); //variab needed for initializing phant
            if (numZ > 0) {
                extract_data_for_dicomdose(dicom[0]);
            }

            for (int i = 0; i < dicom.size(); i++) {
                rescaleFlag = 0;
                for (int j = 0; j < dicom[i]->data.size(); j++) {
                    // Pixel Spacing (Decimal String), row spacing and then column spacing (in mm)
                    if (dicom[i]->data[j]->tag[0] == 0x0028 && dicom[i]->data[j]->tag[1] == 0x0030) {
                        xySpacing.resize(xySpacing.size()+1);
                        xySpacing.last().resize(2);

                        QString temp = "";
                        for (unsigned int s = 0; s < dicom[i]->data[j]->vl; s++) {
                            temp.append(dicom[i]->data[j]->vf[s]);
                        }

                        xySpacing.last()[0] = (temp.split('\\',QString::SkipEmptyParts)[0]).toDouble();
                        xySpacing.last()[1] = (temp.split('\\',QString::SkipEmptyParts)[1]).toDouble();
                    } // Slice Thickness (Decimal String, in mm)
                    else if (dicom[i]->data[j]->tag[0] == 0x0018 && dicom[i]->data[j]->tag[1] == 0x0050) {
                        QString temp = "";
                        for (unsigned int s = 0; s < dicom[i]->data[j]->vl; s++) {
                            temp.append(dicom[i]->data[j]->vf[s]);
                        }

                        zSpacing.append(temp.toDouble());
                    } // Image Position [x,y,z] (Decimal String, in mm)
                    else if (dicom[i]->data[j]->tag[0] == 0x0020 && dicom[i]->data[j]->tag[1] == 0x0032) {
                        imagePos.resize(imagePos.size()+1);
                        imagePos.last().resize(3);

                        QString temp = "";
                        for (unsigned int s = 0; s < dicom[i]->data[j]->vl; s++) {
                            temp.append(dicom[i]->data[j]->vf[s]);
                        }

                        imagePos.last()[0] = (temp.split('\\',QString::SkipEmptyParts)[0]).toDouble();
                        imagePos.last()[1] = (temp.split('\\',QString::SkipEmptyParts)[1]).toDouble();
                        imagePos.last()[2] = (temp.split('\\',QString::SkipEmptyParts)[2]).toDouble();
                    } // Rows
                    else if (dicom[i]->data[j]->tag[0] == 0x0028 && dicom[i]->data[j]->tag[1] == 0x0010) {
                        if (dicom[i]->isBigEndian)
                            xPix.append((unsigned short int)(((short int)(dicom[i]->data[j]->vf[0]) << 8) +
                                                             (short int)(dicom[i]->data[j]->vf[1])));
                        else
                            xPix.append((unsigned short int)(((short int)(dicom[i]->data[j]->vf[1]) << 8) +
                                                             (short int)(dicom[i]->data[j]->vf[0])));
                    } // Columns
                    else if (dicom[i]->data[j]->tag[0] == 0x0028 && dicom[i]->data[j]->tag[1] == 0x0011) {
                        if (dicom[i]->isBigEndian)
                            yPix.append((unsigned short int)(((short int)(dicom[i]->data[j]->vf[0]) << 8) +
                                                             (short int)(dicom[i]->data[j]->vf[1])));
                        else
                            yPix.append((unsigned short int)(((short int)(dicom[i]->data[j]->vf[1]) << 8) +
                                                             (short int)(dicom[i]->data[j]->vf[0])));
                    } // Rescale HU slope (assuming type is HU)
                    else if (dicom[i]->data[j]->tag[0] == 0x0028 && dicom[i]->data[j]->tag[1] == 0x1053) {
                        QString temp = "";
                        for (unsigned int s = 0; s < dicom[i]->data[j]->vl; s++) {
                            temp.append(dicom[i]->data[j]->vf[s]);
                        }

                        rescaleM = temp.toDouble();
                        rescaleFlag++;
                    } // Rescale HU intercept (assuming type is HU)
                    else if (dicom[i]->data[j]->tag[0] == 0x0028 && dicom[i]->data[j]->tag[1] == 0x1052) {
                        QString temp = "";
                        for (unsigned int s = 0; s < dicom[i]->data[j]->vl; s++) {
                            temp.append(dicom[i]->data[j]->vf[s]);
                        }

                        rescaleB = temp.toDouble();
                        rescaleFlag++;
                    } // HU values (assuming 2-bytes as I have yet to encounter anything different, ie, assumes TAG (0028,0100) = 16)
                    else if (dicom[i]->data[j]->tag[0] == 0x7fe0 && dicom[i]->data[j]->tag[1] == 0x0010) {
                        HU.resize(HU.size()+1);
                        if (HU.size() == xPix.size() && HU.size() == yPix.size()) {
                            HU.last().resize(yPix.last());
                            for (unsigned int k = 0; k < yPix.last(); k++) {
                                HU.last()[k].resize(xPix.last());
                            }

                            short int temp;
                            if (dicom[i]->isBigEndian)
                                for (unsigned int s = 0; s < dicom[i]->data[j]->vl; s+=2) {
                                    temp  = (dicom[i]->data[j]->vf[s+1]);
                                    temp += (short int)(dicom[i]->data[j]->vf[s]) << 8;

                                    HU.last()[int(int(s/2)/xPix.last())][int(s/2)%xPix.last()] =
                                        rescaleFlag == 2 ? rescaleM*temp+rescaleB : temp;
                                }
                            else
                                for (unsigned int s = 0; s < dicom[i]->data[j]->vl; s+=2) {
                                    temp  = (dicom[i]->data[j]->vf[s]);
                                    temp += (short int)(dicom[i]->data[j]->vf[s+1]) << 8;

                                    HU.last()[int(int(s/2)/xPix.last())][int(s/2)%xPix.last()] =
                                        rescaleFlag == 2 ? rescaleM*temp+rescaleB : temp;
                                }
                        }
                    }
                }
                updateProgress(increment);
            }

            if (HU.size() > 0) {
                duration = (std::clock()-start)/(double)CLOCKS_PER_SEC;
                std::cout << "Extracted all HU data for the " << xPix[0] << "x" << yPix[0] << " slices.  Time elapsed is " << duration << " s.\n";
            }
            else {
                std::cout << "Did not find HU data.\n";
            }

        }

        // ---------------------------------------------------------- //
        // FETCH RS FILE STRUCTURE DATA                               //
        // ---------------------------------------------------------- //
        if (loadedstruct) {
            // Data for parsing singly and doubly nested SQ sets and point data strings
            QVector <Attribute *> *att, *att2;
            QVector<QString> ROI_type;
            QByteArray tempData, tempData2;
            QStringList pointData;
            QVector<int> structNum_roiSequence;

            QVector <QVector <cont>> contourPoints;

            for (int j = 0; j < dicomStruct->data.size(); j++) {
                //Save data to dicomHeader (used to generate the dicom dose file)
                if (dicomStruct->data[j]->tag[0] == 0x0008 && dicomStruct->data[j]->tag[1] == 0x0016) { //SOP Class UID
                    QString temp = "";
                    for (unsigned int s = 0; s < dicomStruct->data[j]->vl; s++) {
                        temp.append(dicomStruct->data[j]->vf[s]);
                    }
                    dicomHeader.insert("00081150", temp); //A reference to the struct UID

                }
                else if (dicomStruct->data[j]->tag[0] == 0x0008 && dicomStruct->data[j]->tag[1] == 0x0018) {   //SOP Instance UID
                    QString temp = "";
                    for (unsigned int s = 0; s < dicomStruct->data[j]->vl; s++) {
                        temp.append(dicomStruct->data[j]->vf[s]);
                    }
                    dicomHeader.insert("00081155", temp); //A reference to the struct UID
                } // Structure info (looking for structure names and nums)
                else if (dicomStruct->data[j]->tag[0] == 0x3006 && dicomStruct->data[j]->tag[1] == 0x0020) {
                    for (int k = 0; k < dicomStruct->data[j]->seq.items.size(); k++) {
                        tempData = QByteArray((char *)dicomStruct->data[j]->seq.items[k]->vf,dicomStruct->data[j]->seq.items[k]->vl);
                        QDataStream dataStream(tempData);

                        att = new QVector <Attribute *>;
                        if (!dicomStruct->parseSequence(&dataStream, att)) {
                            std::cout << "Failed to parse sequence data for tag (3006,0020), contour structure info \n";
                        }

                        QString tempS = ""; // Get the name
                        QString tempI = ""; // Get the number
                        for (int l = 0; l < att->size(); l++) {
                            if (att->at(l)->tag[0] == 0x3006 && att->at(l)->tag[1] == 0x0026)
                                for (unsigned int s = 0; s < att->at(l)->vl; s++) {
                                    tempS.append(att->at(l)->vf[s]);
                                }
                            else if (att->at(l)->tag[0] == 0x3006 && att->at(l)->tag[1] == 0x0022)
                                for (unsigned int s = 0; s < att->at(l)->vl; s++) {
                                    tempI.append(att->at(l)->vf[s]);
                                }
                        }

                        structName.append(tempS.trimmed());
                        structNum.append(tempI.toInt());
                        structLookup[tempI.toInt()] = structName.size()-1;

                        for (int l = 0; l < att->size(); l++) {
                            delete att->at(l);
                        }
                        delete att;
                    }
                } // Structure data (looking for contour definitions)
                else if (dicomStruct->data[j]->tag[0] == 0x3006 && dicomStruct->data[j]->tag[1] == 0x0039) {
                    for (int k = 0; k < dicomStruct->data[j]->seq.items.size(); k++) {
                        tempData = QByteArray((char *)dicomStruct->data[j]->seq.items[k]->vf,dicomStruct->data[j]->seq.items[k]->vl);
                        QDataStream dataStream(tempData);

                        att = new QVector <Attribute *>;
                        if (!dicomStruct->parseSequence(&dataStream, att)) {
                            std::cout << "Failed to parse sequence data for tag (3006,0039), contour structure data \n";
                        }

                        QString tempS = ""; // Get the contour, it's another nested sequence, so we must go deeper with parseSequence
                        for (int l = 0; l < att->size(); l++) {
                            if (att->at(l)->tag[0] == 0x3006 && att->at(l)->tag[1] == 0x0040) {
                                structZ.resize(structZ.size()+1);
                                structPos.resize(structPos.size()+1);
                                contourPoints.resize(contourPoints.size()+1);
                                for (int k = 0; k < att->at(l)->seq.items.size(); k++) {

                                    structPos.last().resize(structPos.last().size()+1);
                                    tempData2 = QByteArray((char *)att->at(l)->seq.items[k]->vf,att->at(l)->seq.items[k]->vl);
                                    QDataStream dataStream2(tempData2);

                                    att2 = new QVector <Attribute *>;
                                    if (!dicomStruct->parseSequence(&dataStream2, att2)) {
                                        std::cout << "Failed to parse sequence data for tag (3006,0040), contour data points \n";
                                    }

                                    QString tempS = ""; // Get the points
                                    for (int m = 0; m < att2->size(); m++)
                                        if (att2->at(m)->tag[0] == 0x3006 && att2->at(m)->tag[1] == 0x0050)
                                            for (unsigned int s = 0; s < att2->at(m)->vl; s++) {
                                                tempS.append(att2->at(m)->vf[s]);
                                            }

                                    pointData = tempS.split('\\');

                                    structZ.last().append(pointData[2].toDouble()/10.0);


                                    for (int m = 0; m < pointData.size(); m+=3) {
                                        structPos.last().last() << QPointF(pointData[m].toDouble()/10.0, pointData[m+1].toDouble()/10.0);
                                        cont point;
                                        point.x = pointData[m].toDouble()/10.0 ;
                                        point.y = pointData[m+1].toDouble()/10.0;
                                        point.z = pointData[m+2].toDouble()/10.0;
                                        contourPoints.last().append(point);
                                    }


                                    for (int m = 0; m < att2->size(); m++) {
                                        delete att2->at(m);
                                    }
                                    delete att2;
                                }

                            }
                            else if (att->at(l)->tag[0] == 0x3006 && att->at(l)->tag[1] == 0x0084) {
                                QString tempI = ""; // Get the number
                                for (unsigned int s = 0; s < att->at(l)->vl; s++) {
                                    tempI.append(att->at(l)->vf[s]);
                                }
                                structReference.append(tempI.toInt());

                            }
                        }

                        for (int l = 0; l < att->size(); l++) {
                            delete att->at(l);
                        }
                        delete att;
                    }
                } //RT ROI Observation Sequence
                else if (dicomStruct->data[j]->tag[0] == 0x3006 && dicomStruct->data[j]->tag[1] == 0x0080) {
                    for (int k = 0; k < dicomStruct->data[j]->seq.items.size(); k++) {
                        tempData = QByteArray((char *)dicomStruct->data[j]->seq.items[k]->vf,dicomStruct->data[j]->seq.items[k]->vl);
                        QDataStream dataStream(tempData);

                        att = new QVector <Attribute *>;
                        if (!dicomStruct->parseSequence(&dataStream, att)) {
                            std::cout << "Failed to parse sequence data for tag (3006,0080)\n";
                            //return 0;
                        }

                        QString tempS = ""; // Get the name
                        QString tempI = ""; // Get the number
                        for (int l = 0; l < att->size(); l++) {
                            if (att->at(l)->tag[0] == 0x3006 && att->at(l)->tag[1] == 0x0084)
                                for (unsigned int s = 0; s < att->at(l)->vl; s++) {
                                    tempS.append(att->at(l)->vf[s]);
                                }
                            else if (att->at(l)->tag[0] == 0x3006 && att->at(l)->tag[1] == 0x00a4)
                                for (unsigned int s = 0; s < att->at(l)->vl; s++) {
                                    tempI.append(att->at(l)->vf[s]);
                                }

                        }

                        ROI_type.append(tempI.trimmed());
                        structNum_roiSequence.append(tempS.toInt());

                        for (int l = 0; l < att->size(); l++) {
                            delete att->at(l);
                        }
                        delete att;
                    }
                }
                updateProgress(increment);
            }


            //If the RT ROI interepreted type is 'BRACHY_CHANNEL', remove the structure
            //If its external, remove and save to seperate external structure
            for (int i=0; i<ROI_type.size(); i++) {
                if (ROI_type[i] == "BRACHY_CHANNEL") {
                    for (int j=0; j<structReference.size(); j++) {
                        if (structReference[j] == structNum_roiSequence[i]) {
                            structPos.remove(j);
                            structZ.remove(j);
                            contourPoints.remove(j);
                            structReference.remove(j);
                        }
                    }
                    for (int k=0; k<structNum.size(); k++) {
                        if (structNum[k] == structNum_roiSequence[i]) {
                            structName.remove(k);
                            if (structLookup.contains(structNum[k])) {
                                structLookup.remove(structNum[k]);
                            }
                            structNum.remove(k);
                        }
                    }
                }
                else  {
                    structType.append(ROI_type[i]);
                    if (ROI_type[i] == "EXTERNAL") {
                        externalStruct = true;
                        indexExternal.append(i); //index of all external media
                    }
                }
            }


            //If there is no contour data (its empty), remove it
            // need to remove the structure name and lookup
            int numEmpty = 0;
            for (int i=0; i<structZ.size(); i++) {
                if (structZ[i].isEmpty()) {
                    numEmpty++;
                    for (int j=0; j<structReference.size(); j++) {
                        if (structReference[i] == structNum_roiSequence[j]) {
                            structType.remove(j);
                            ROI_type.remove(j);
                        }
                    }
                    for (int k=0; k<structNum.size(); k++) {
                        if (structNum[k] == structReference[i]) {
                            structName.remove(k);
                            if (structLookup.contains(structNum[k])) {
                                structLookup.remove(structNum[k]);
                            }
                            structNum.remove(k);
                        }
                    }
                    structPos.remove(i);
                    structZ.remove(i);
                    contourPoints.remove(i);
                    structReference.remove(i);
					structName.remove(i);
                }
            }
            if (numEmpty > 0) {
                std::cout << numEmpty <<" structures without contour data points were removed \n";
            }

            //Change the structLookup since data was deleted
            for (int k=0; k<structNum.size(); k++) {
                structLookup[structNum[k]] = k;
            }


            //Getting the contour exterma
            QVector <double> bounds(2, 0);
            QVector <QVector <double> > contour(3, bounds);
            QVector <QVector <QVector <double> > > etrm(structName.size(), contour);
            extrema = etrm;
			

            for (int i=0; i<structZ.size(); i++) {
                double xmin = contourPoints[i][0].x, xmax = contourPoints[i][0].x;
                double ymin = contourPoints[i][0].y, ymax = contourPoints[i][0].y;
                double zmin = contourPoints[i][0].z, zmax = contourPoints[i][0].z;

                for (int j=0; j<contourPoints[i].size(); j++) {
					if (contourPoints[i][j].x > xmax) {
                        xmax = contourPoints[i][j].x;
                    }
                    if (contourPoints[i][j].x < xmin) {
                        xmin = contourPoints[i][j].x;
                    }
                    if (contourPoints[i][j].y > ymax) {
                        ymax = contourPoints[i][j].y;
                    }
                    if (contourPoints[i][j].y < ymin) {
                        ymin = contourPoints[i][j].y;
                    }
                    if (contourPoints[i][j].z > zmax) {
                        zmax = contourPoints[i][j].z;
                    }
                    if (contourPoints[i][j].z < zmin) {
                        zmin = contourPoints[i][j].z;
                    }
                }
                extrema[i][0][0] = xmin;
                extrema[i][0][1] = xmax;
                extrema[i][1][0] = ymin;
                extrema[i][1][1] = ymax;
                extrema[i][2][0] = zmin;
                extrema[i][2][1] = zmax;
            }


            duration = (std::clock()-start)/(double)CLOCKS_PER_SEC;
            std::cout << "Extracted data for all " << structName.size() << " structures.  Time elapsed is " << duration << " s.\n";


        }

        // ---------------------------------------------------------- //
        // FETCH RP FILE PLAN DATA                                   //
        // ---------------------------------------------------------- //
        if (loadedplan) {

            QVector <Attribute *> *att, *att2;
            QByteArray tempData, tempData2;
            double half_life = 0;
            //double t_end = 0; // Unused
            double max_time = 0;


            // Data for parsing singly and doubly nested SQ sets and point data strings
            QVector <Attribute *> *att3;
            QByteArray  tempData3;
            QStringList pointData2;


            for (int j = 0; j < dicomPlan->data.size(); j++) {
                //Save data to dicomHeader (used to generate the dicom dose file)
                if (dicomPlan->data[j]->tag[0] == 0x0008 && dicomPlan->data[j]->tag[1] == 0x0016) { //SOP Class UID
                    QString temp = "";
                    for (unsigned int s = 0; s < dicomPlan->data[j]->vl; s++) {
                        temp.append(dicomPlan->data[j]->vf[s]);
                    }
                    if (dicomHeader.contains("00081150")) { //Data from both the plan and struct file (plan is first)
                        QString value = dicomHeader.value("00081150");
                        QString new_value = temp + "\\" + value;
                        dicomHeader.insert("00081150", new_value);
                    }
                    else {
                        dicomHeader.insert("00081150", temp); //A reference to the struct UID
                    }
                }
                else if (dicomPlan->data[j]->tag[0] == 0x0008 && dicomPlan->data[j]->tag[1] == 0x0018) {   //SOP Instance UID (plan is first)
                    QString temp = "";
                    for (unsigned int s = 0; s < dicomPlan->data[j]->vl; s++) {
                        temp.append(dicomPlan->data[j]->vf[s]);
                    }
                    if (dicomHeader.contains("00081155")) { //Data from both the plan and struct file
                        QString value = dicomHeader.value("00081155");
                        QString new_value = temp + "\\" + value;
                        dicomHeader.insert("00081155", new_value);
                    }
                    else {
                        dicomHeader.insert("00081155", temp); //A reference to the struct UID
                    }
                } // Treatment Type
                else if (dicomPlan->data[j]->tag[0] == 0x300a && dicomPlan->data[j]->tag[1] == 0x0202) {
                    QString temp = "";
                    for (unsigned int s = 0; s < dicomPlan->data[j]->vl; s++) {
                        temp.append(dicomPlan->data[j]->vf[s]);
                    }
                    treatment_type = temp.trimmed();

                } //Treatment technique
                else if (dicomPlan->data[j]->tag[0] == 0x300a && dicomPlan->data[j]->tag[1] == 0x0200) {
                    QString temp = "";
                    for (unsigned int s = 0; s < dicomPlan->data[j]->vl; s++) {
                        temp.append(dicomPlan->data[j]->vf[s]);
                    }

                    treatment_technique = temp.trimmed();
                } //Source sequence
                else if (dicomPlan->data[j]->tag[0] == 0x300a && dicomPlan->data[j]->tag[1] == 0x0210) {
                    for (int k = 0; k < dicomPlan->data[j]->seq.items.size(); k++) {
                        tempData = QByteArray((char *)dicomPlan->data[j]->seq.items[k]->vf,dicomPlan->data[j]->seq.items[k]->vl);
                        QDataStream dataStream(tempData);

                        att = new QVector <Attribute *>;
                        if (!dicomPlan->parseSequence(&dataStream, att)) {
                            std::cout << "Failed to parse sequence data for tag (300a,0210), Source Sequence\n";
                        }

                        QString tempS = ""; // Get the air kerma
                        QString tempI = ""; // Get the half life
                        QString tempE = ""; //Get the isotope name

                        for (int l = 0; l < att->size(); l++) {
                            if (att->at(l)->tag[0] == 0x300a && att->at(l)->tag[1] == 0x022a)
                                for (unsigned int s = 0; s < att->at(l)->vl; s++) {
                                    tempS.append(att->at(l)->vf[s]);
                                }
                            else if (att->at(l)->tag[0] == 0x300a && att->at(l)->tag[1] == 0x0228)
                                for (unsigned int s = 0; s < att->at(l)->vl; s++) {
                                    tempI.append(att->at(l)->vf[s]);
                                }
                            else if (att->at(l)->tag[0] == 0x300a && att->at(l)->tag[1] == 0x0226)
                                for (unsigned int s = 0; s < att->at(l)->vl; s++) {
                                    tempE.append(att->at(l)->vf[s]);
                                }
                            else if (att->at(l)->tag[0] % 2 != 0) { //look for odd tags - private elements and save the data to search for seed
                                QString temp = "";
                                for (unsigned int s = 0; s < att->at(l)->vl; s++) {
                                    temp.append(att->at(l)->vf[s]);
                                }
                                Seed_info.append(temp);

                            }
                        }

                        air_kerma = tempS.toDouble();
                        half_life = tempI.toDouble();
                        isotope_name = tempE.trimmed();

                        for (int l = 0; l < att->size(); l++) {
                            delete att->at(l);
                        }
                        delete att;
                    }
                } // Application Setup Sequence (looking for control point position)
                else if (dicomPlan->data[j]->tag[0] == 0x300a && dicomPlan->data[j]->tag[1] == 0x0230) {

                    for (int k = 0; k < dicomPlan->data[j]->seq.items.size(); k++) {
                        tempData = QByteArray((char *)dicomPlan->data[j]->seq.items[k]->vf,dicomPlan->data[j]->seq.items[k]->vl);
                        QDataStream dataStream(tempData);

                        att = new QVector <Attribute *>;
                        if (!dicomPlan->parseSequence(&dataStream, att)) {
                            std::cout << "Failed to parse sequence data for tag (300a,0230),  source information\n";
                            //return 0;
                        }

                        QString tempS = ""; // Get the channel, it's another nested sequence, so we must go deeper with parseSequence
                        for (int l = 0; l < att->size(); l++) {
                            if (att->at(l)->tag[0] == 0x300a && att->at(l)->tag[1] == 0x0280) {
                                double final_cumulative_time_weight =0.0, channel_total_time = 0.0;

                                //std::cout<<" Found 300a, 0280, new sequence \n";
                                for (int k = 0; k < att->at(l)->seq.items.size(); k++) {
                                    tempData2 = QByteArray((char *)att->at(l)->seq.items[k]->vf,att->at(l)->seq.items[k]->vl);
                                    QDataStream dataStream2(tempData2);

                                    att2 = new QVector <Attribute *>;
                                    if (!dicomPlan->parseSequence(&dataStream2, att2)) {
                                        std::cout << "Failed to parse sequence data for tag (300a,0280) \n";
                                    }

                                    QString tempS = ""; // Get the brachy sequence, it's another nested sequence, so we must go deeper with parseSequence
                                    for (int m = 0; m < att2->size(); m++) {
                                        if (att2->at(m)->tag[0] == 0x300a && att2->at(m)->tag[1] == 0x02d0) { //Control sequence
                                            QVector<coordinate> dwell_position;
                                            QVector<double> dwell_time;
                                            QVector<coordinate> position;
                                            QVector <double> cumulative_time_weight;

                                            for (int k2 = 0; k2 < att2->at(m)->seq.items.size(); k2++) {
                                                tempData3 = QByteArray((char *)att2->at(m)->seq.items[k2]->vf,att2->at(m)->seq.items[k2]->vl);
                                                QDataStream dataStream3(tempData3);

                                                att3 = new QVector <Attribute *>;
                                                if (!dicomPlan->parseSequence(&dataStream3, att3)) {
                                                    std::cout << "Failed to parse sequence data for tag (300a,02d0), source points data \n";
                                                }


                                                QString tempS = ""; // Get the points
                                                QString tempT = "";
                                                for (int n = 0; n < att3->size(); n++) {
                                                    if (att3->at(n)->tag[0] == 0x300a && att3->at(n)->tag[1] == 0x02d4) { //Seed position

                                                        for (unsigned int s = 0; s < att3->at(n)->vl; s++) {
                                                            tempS.append(att3->at(n)->vf[s]);
                                                        }

                                                        pointData2 = tempS.split('\\');
                                                        for (int p = 0; p < pointData2.size(); p+=3) {
                                                            coordinate pos;
                                                            pos.x = pointData2[p].toDouble()/10.0;
                                                            pos.y = pointData2[p+1].toDouble()/10.0;
                                                            pos.z = pointData2[p+2].toDouble()/10.0;

                                                            position.append(pos);
                                                        }
                                                    }
                                                    else if (att3->at(n)->tag[0] == 0x300a && att3->at(n)->tag[1] == 0x02d6) { //Seed time weight

                                                        for (unsigned int s = 0; s < att3->at(n)->vl; s++) {
                                                            tempT.append(att3->at(n)->vf[s]);
                                                        }

                                                        cumulative_time_weight.append(tempT.toDouble());

                                                    }
                                                }
                                                for (int n = 0; n < att3->size(); n++) {
                                                    delete att3->at(n);
                                                }
                                                delete att3;
                                            }
                                            //Calculate the dwell position
                                            for (int q=1; q< position.size(); q++) {
                                                if ((position[q].x == position[q-1].x)&&(position[q].y == position[q-1].y)&&(position[q].z == position[q-1].z)) {
                                                    if (((cumulative_time_weight[q] - cumulative_time_weight[q-1]) > 0) || treatment_technique == "PERMANENT") {
                                                        dwell_position.append(position[q]);
                                                        dwell_time.append((channel_total_time*(cumulative_time_weight[q]-cumulative_time_weight[q-1]))/final_cumulative_time_weight);

                                                        //times for HDR
                                                        if (dwell_time.last() > max_time) {
                                                            max_time = dwell_time.last();
                                                        }
                                                        total_time += dwell_time.last();

                                                    }
                                                }
                                            }

                                            all_seed_pos.append(dwell_position);
                                            all_seed_time.append(dwell_time);


                                        } //Culmulative time weight
                                        else if (att2->at(m)->tag[0] == 0x300a && att2->at(m)->tag[1] == 0x02c8) {
                                            QString tempS = "";
                                            for (unsigned int s = 0; s < att2->at(m)->vl; s++) {
                                                tempS.append(att2->at(m)->vf[s]);
                                            }
                                            final_cumulative_time_weight = tempS.toDouble();

                                        } //Channel total time
                                        else if (att2->at(m)->tag[0] == 0x300a && att2->at(m)->tag[1] == 0x0286) {
                                            QString tempS = "";
                                            for (unsigned int s = 0; s < att2->at(m)->vl; s++) {
                                                tempS.append(att2->at(m)->vf[s]);
                                            }
                                            channel_total_time= tempS.toDouble();

                                        }
                                    }
									for (int n = 0; n < att2->size(); n++) {
										delete att2->at(n);
									}
									delete att2;
                                }
                            }
                        }
                    }
                }
                updateProgress(increment);
            }

            double tau = (half_life/log(2))*24; //mean lifetime in hours (as air kerma is measured in hours), (half life is in days)

            if (treatment_technique != "PERMANENT" && treatment_type == "LDR") { //LDR DSF //ie eye plaque where implant comes out
                DSF_intermediate = (air_kerma)*tau*(1- exp(-(1/tau)*total_time));
            }
            else if (treatment_technique == "PERMANENT" && (treatment_type == "LDR" || treatment_type == "MANUAL")) {
                DSF_intermediate = (air_kerma)*tau;
            }
            else if (treatment_type == "HDR") { //HDR DSF
                DSF_intermediate = (air_kerma*(total_time/3600)); //Value is multiplied by max weight when it is calculated
            }
            else {      //default, shouldnt go here
                DSF_intermediate = air_kerma*tau;
                std::cout<<"Warning: default dose scaling factor calculated (air kerma* radionuclide mean lifetim) as treatment technique couldn't be found in the DICOM plan file \n";
            }
            std::cout << "Extracted data for all " << treatment_type.toStdString() << " brachytherapy seeds. \n";

        }

        progress2->setValue(1000000000);
        progWin2->hide();

    }
    else {
        progress2->setValue(1000000000);
        progWin2->hide();
    }

}



/***
Function: extract_data_for_dicomdose
-------------------
Process: This function extract data from relevant tags in the input dicom file
        such that it can be used to create the DICOM dose file
***/
void DICOM::extract_data_for_dicomdose(DICOM *dicom) {

    for (int i = 0; i < dicom->data.size(); i++) {

        if (dicom->data[i]->tag[0] == 0x0008 && dicom->data[i]->tag[1] == 0x0050) { //Accession Number attribute
            QString temp = "";
            for (unsigned int s = 0; s < dicom->data[i]->vl; s++) {
                temp.append(dicom->data[i]->vf[s]);
            }
            dicomHeader.insert("00080050", temp);
        }
        else if (dicom->data[i]->tag[0] == 0x0010 && dicom->data[i]->tag[1] == 0x0010) {   //Patient Name attribute
            QString temp = "";
            for (unsigned int s = 0; s < dicom->data[i]->vl; s++) {
                temp.append(dicom->data[i]->vf[s]);
            }
            dicomHeader.insert("00100010", temp);
        }
        else if (dicom->data[i]->tag[0] == 0x0010 && dicom->data[i]->tag[1] == 0x0020) {   //Patient Id attribute
            QString temp = "";
            for (unsigned int s = 0; s < dicom->data[i]->vl; s++) {
                temp.append(dicom->data[i]->vf[s]);
            }
            dicomHeader.insert("00100020", temp);
        }
        else if (dicom->data[i]->tag[0] == 0x0010 && dicom->data[i]->tag[1] == 0x0030) {   //Patient Birth date attribute
            QString temp = "";
            for (unsigned int s = 0; s < dicom->data[i]->vl; s++) {
                temp.append(dicom->data[i]->vf[s]);
            }
            dicomHeader.insert("00100030", temp);
        }
        else if (dicom->data[i]->tag[0] == 0x0010 && dicom->data[i]->tag[1] == 0x0040) {   //Patient Sex attribute
            QString temp = "";
            for (unsigned int s = 0; s < dicom->data[i]->vl; s++) {
                temp.append(dicom->data[i]->vf[s]);
            }
            dicomHeader.insert("00100040", temp);
        }
        else if (dicom->data[i]->tag[0] == 0x0020 && dicom->data[i]->tag[1] == 0x000d) {   //Stude Instance UID attribute
            QString temp = "";
            for (unsigned int s = 0; s < dicom->data[i]->vl; s++) {
                temp.append(dicom->data[i]->vf[s]);
            }
            dicomHeader.insert("0020000d", temp);
        }
        else if (dicom->data[i]->tag[0] == 0x0020 && dicom->data[i]->tag[1] == 0x0052) {   //Frame of reference UID attribute
            QString temp = "";
            for (unsigned int s = 0; s < dicom->data[i]->vl; s++) {
                temp.append(dicom->data[i]->vf[s]);
            }
            dicomHeader.insert("00200052", temp);
        }
        else if (dicom->data[i]->tag[0] == 0x0008 && dicom->data[i]->tag[1] == 0x0090) {   //Physician Name attribute
            QString temp = "";
            for (unsigned int s = 0; s < dicom->data[i]->vl; s++) {
                temp.append(dicom->data[i]->vf[s]);
            }
            dicomHeader.insert("00080090", temp);
        }
        else if (dicom->data[i]->tag[0] == 0x0008 && dicom->data[i]->tag[1] == 0x0070) {   //Manufacturer attribute
            QString temp = "";
            for (unsigned int s = 0; s < dicom->data[i]->vl; s++) {
                temp.append(dicom->data[i]->vf[s]);
            }
            dicomHeader.insert("00080070", temp);
        }
    }

}


/***
Function: loadCalib
-------------------
Process: This function allows the user to select and load calibration file
***/
void Interface::loadCalib() {

    QString fileName = QFileDialog::getOpenFileName(
                           this,
                           tr("Select the CT Calibration File to open"),
                           working_path, "hu2rho (*.hu2rho)");

    if (!fileName.isEmpty()) {
        density = fileName;

        if (!readCalib()) {
            do {
                QMessageBox msgBox;
                msgBox.setText(tr("The CT calibration file could not be read. Please select a valid hu2rho file.\nThe user manual contains the hu2rho file specifications."));
                msgBox.setWindowTitle(tr("egs_brachy GUI"));
                msgBox.exec();

                fileName = QFileDialog::getOpenFileName(
                               this,
                               tr("Select a valid CT calibration file"),
                               working_path, "hu2rho (*.hu2rho)");

                if (!fileName.isEmpty()) {
                    density = fileName;
                    readCalib();
                }

            }
            while (!readCalibFile && !fileName.isEmpty() && !fileName.isNull());
        }

        if (!readCalibFile) { //still have't read the file - user clicked cancel so we go back to the default
            density = working_path + "/default_CT_calib.hu2rho";    //re-assign the default
        }
        else {
            std::cout<<"Successfully read CT calibration file " <<density.toStdString() <<"\n";
        }
    }
}

bool Interface::readCalib() {
    // ---------------------------------------------------------- //
    // READ THE HU TO DENSITY CONVERSION FILE                     //
    // ---------------------------------------------------------- //
    readCalibFile = false; //reset the flag
    denMap.clear();
    HUMap.clear();
    QFile *file = new QFile(density);
    QString tempS = "";
    int countLine = 0;
    if (file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream input(file);

        while (!input.atEnd()) {
            tempS = input.readLine().trimmed();
            countLine ++;
            if (tempS.contains(QRegExp("\\s+"))) {
                HUMap << tempS.split(QRegExp("\\s+"), QString::SkipEmptyParts)[0].trimmed().toDouble();
                denMap<< tempS.split(QRegExp("\\s+"), QString::SkipEmptyParts)[1].trimmed().toDouble();

            }
            else {
                std::cout<<"Error reading CT calibration file " << density.toStdString() << ". Did not find a whitespace delimiter on line " <<countLine   <<".\n";
                return false;
            }

        }

        if (HUMap.size() == denMap.size()) {
            readCalibFile = true;
            return true;
        }
        else {
            std::cout << "Error reading CT calibration file " << density.toStdString() << ". The size of the HU and density vectors do not match.\n";
            return false;
        }
    }
    else {
        std::cout << "Could not open CT calibration file " << density.toStdString() << "\n";
        return false;
    }
    delete file;


}

/***
Function: show_advanced_options
-------------------------------
Process: opens the advanced options window
***/
void Interface::show_advanced_options() {
    //this->setDisabled(true);
    options->save_options_on_startup(egs_input->egsinp_path);
    options->window->show();
    QApplication::processEvents();
    options->setEnabled(TRUE);


}

/***
Function: show_trim_options
-------------------------------
Process: opens the trim egsphant window
***/
void Interface::show_trim_options() {
    connect(trimEGS, SIGNAL(trimExisting()),
            this, SLOT(trimExisitngEGS()));
    connect(trimEGS, SIGNAL(trimNotExisting()),
            this, SLOT(trimPhant_notAlreadyExisting()));

    trimEGS->save_options_on_startup("");
    trimEGS->egsphantPath = egs_input->egsphant_location;
    trimEGS->window->show();
    QApplication::processEvents();
    trimEGS->setEnabled(TRUE);

}


/***
Function: trimExisitngEGS
-------------------------------
Process: trims the egsphant if it has already been created
***/
void Interface::trimExisitngEGS() {
    //Delete density, media and contour indicies
    for (int i = phant.nx - 1; i >= 0; i--) {
        if (i < trimEGS->xMinIndex || i > trimEGS->xMaxIndex) {
            phant.m.remove(i);
            phant.d.remove(i);
            phant.contour.remove(i);
        }
        else {
            for (int j = phant.ny-1; j>= 0; j--) {
                if (j < trimEGS->yMinIndex || j > trimEGS->yMaxIndex) {
                    phant.m[i].remove(j);
                    phant.d[i].remove(j);
                    phant.contour[i].remove(j);
                }
                else {
                    for (int k = phant.nz - 1; k >= 0; k--)
                        if (k < trimEGS->zMinIndex || k > trimEGS->zMaxIndex) {
                            phant.m[i][j].remove(k);
                            phant.d[i][j].remove(k);
                            phant.contour[i][j].remove(k);
                        }
                }
            }
        }
    }

    //Resize
    for (int k = phant.nz; k >= 0; k--)
        if (k < trimEGS->zMinIndex || k > trimEGS->zMaxIndex) {
            phant.z.remove(k);
        }

    for (int k = phant.ny; k >= 0; k--)
        if (k < trimEGS->yMinIndex || k > trimEGS->yMaxIndex) {
            phant.y.remove(k);
        }

    for (int k = phant.nx ; k >= 0; k--)
        if (k < trimEGS->xMinIndex || k > trimEGS->xMaxIndex) {
            phant.x.remove(k);
        }


    phant.nx = phant.x.size() -1;
    phant.ny = phant.y.size() -1;
    phant.nz = phant.z.size() -1;

    //Change the trim boundaries as data was deleted
    trimEGS->x = phant.x;
    trimEGS->y = phant.y;
    trimEGS->z = phant.z;
    trimEGS->reset_bounds();


    phant.saveEGSPhantFile(egs_input->egsphant_location);
    QProcess *qzip = new QProcess();
    qzip->setProcessChannelMode(QProcess::MergedChannels);
    qzip->setReadChannel(QProcess::StandardOutput);
    QString command = "gzip -f " + egs_input->egsphant_location;  //-f used if exisitng .gz file, -f forces the
    qzip->execute(command);
    std::cout << "Trimmed egsphant file successfully output." <<egs_input->egsphant_location.toStdString() <<".gz  \n";

    disconnect(trimEGS, SIGNAL(trimExisting()),
               this, SLOT(trimExisitngEGS()));
    disconnect(trimEGS, SIGNAL(trimNotExisting()),
               this, SLOT(trimPhant_notAlreadyExisting()));
}

/***
Function: trimExisitngEGS
-------------------------------
Process: trims the egsphant if it has NOT already been created
                If it already exists, just need to change the phant bounds and indexing
***/
void Interface::trimPhant_notAlreadyExisting() {
    //Delete HU data
    for (int k = phant.nz-1; k >= 0; k--) {
        if (k < trimEGS->zMinIndex || k > trimEGS->zMaxIndex) {
            get_data->HU.remove(k);
        }
        else {
            for (int j = phant.ny-1; j>= 0; j--) {
                if (j < trimEGS->yMinIndex || j > trimEGS->yMaxIndex) {
                    get_data->HU[k].remove(j);
                }
                else {
                    for (int i = phant.nx-1; i >= 0; i--) {
                        if (i < trimEGS->xMinIndex || i > trimEGS->xMaxIndex) {
                            get_data->HU[k][j].remove(i);
                        }
                    }
                }
            }
        }
    }

    //Delete phant bounds
    for (int k = phant.nz; k >= 0; k--)
        if (k < trimEGS->zMinIndex || k > trimEGS->zMaxIndex) {
            phant.z.remove(k);
        }

    for (int k = phant.ny; k >= 0; k--)
        if (k < trimEGS->yMinIndex || k > trimEGS->yMaxIndex) {
            phant.y.remove(k);
        }

    for (int k = phant.nx ; k >= 0; k--)
        if (k < trimEGS->xMinIndex || k > trimEGS->xMaxIndex) {
            phant.x.remove(k);
        }

    //Resize
    phant.nx = phant.x.size() -1;
    phant.ny = phant.y.size() -1;
    phant.nz = phant.z.size() -1;

    QVector <char> mz(phant.nz, 0);
    QVector <QVector <char> > my(phant.ny, mz);
    QVector <QVector <QVector <char> > > mx(phant.nx, my);
    phant.m = mx;

    QVector <double> dz(phant.nz, 0);
    QVector <QVector <double> > dy(phant.ny, dz);
    QVector <QVector <QVector <double> > > dx(phant.nx, dy);
    phant.d = dx;

    QVector <int> sz(phant.nz, 0);
    QVector <QVector <int> > sy(phant.ny, sz);
    QVector <QVector <QVector <int> > > sx(phant.nx, sy);
    phant.contour = sx;

    //Change the trim boundaries as data was deleted
    trimEGS->x = phant.x;
    trimEGS->y = phant.y;
    trimEGS->z = phant.z;
    trimEGS->reset_bounds();

    std::cout << "Egsphant has been trimmed (dimensions x: [" << phant.x[0] << "," << phant.x[phant.nx] << "], y:["
              << phant.y[0] << "," << phant.y[phant.ny] << "], z:["
              << phant.z[0] << "," << phant.z[phant.nz] << "]) \n";

    disconnect(trimEGS, SIGNAL(trimExisting()),
               this, SLOT(trimExisitngEGS()));
    disconnect(trimEGS, SIGNAL(trimNotExisting()),
               this, SLOT(trimPhant_notAlreadyExisting()));
}

/***
Function: trimExisitngEGS_Mid
-------------------------------
Process: trims the egsphant if it has already been created
***/
QString Interface::trimExisitngEGS_Mid(QString phant_file) {
    setup_progress_bar("Parsing the egsphant file", "");
    double increment =  1000000000/(11);
    updateProgress(increment);

    if (phant_file.endsWith(".egsphant.gz")) { //gunzip the file
        updateProgress(increment);
        QProcess *qzip = new QProcess();
        qzip->setProcessChannelMode(QProcess::MergedChannels);
        qzip->setReadChannel(QProcess::StandardOutput);

        //Actual line of exectuion
        QString command = "gunzip " + phant_file;
        qzip->execute(command);
        updateProgress(increment);

        phant_file.chop(3); //remove the .gz from the sting file name
        updateProgress(increment);
    }
    else {
        updateProgress(3*increment);
    }
    updateProgress(increment);

    EGSPhant *selectedPhant= new EGSPhant;
    selectedPhant->loadEGSPhantFilePlus(phant_file);
    updateProgress(increment);

    //Delete density, media and contour indicies
    for (int i = selectedPhant->nx - 1; i >= 0; i--) {
        if (i < trimEGSMid->xMinIndex || i > trimEGSMid->xMaxIndex) {
            selectedPhant->m.remove(i);
            selectedPhant->d.remove(i);
        }
        else {
            for (int j = selectedPhant->ny-1; j>= 0; j--) {
                if (j < trimEGSMid->yMinIndex || j > trimEGSMid->yMaxIndex) {
                    selectedPhant->m[i].remove(j);
                    selectedPhant->d[i].remove(j);
                }
                else {
                    for (int k = selectedPhant->nz - 1; k >= 0; k--)
                        if (k < trimEGSMid->zMinIndex || k > trimEGSMid->zMaxIndex) {
                            selectedPhant->m[i][j].remove(k);
                            selectedPhant->d[i][j].remove(k);
                        }
                }
            }
        }
    }
    updateProgress(increment);

    //Resize
    for (int k = selectedPhant->nz; k >= 0; k--)
        if (k < trimEGSMid->zMinIndex || k > trimEGSMid->zMaxIndex) {
            selectedPhant->z.remove(k);
        }

    for (int k = selectedPhant->ny; k >= 0; k--)
        if (k < trimEGSMid->yMinIndex || k > trimEGSMid->yMaxIndex) {
            selectedPhant->y.remove(k);
        }

    for (int k = selectedPhant->nx ; k >= 0; k--)
        if (k < trimEGSMid->xMinIndex || k > trimEGSMid->xMaxIndex) {
            selectedPhant->x.remove(k);
        }

    updateProgress(increment);

    selectedPhant->nx = selectedPhant->x.size() -1;
    selectedPhant->ny = selectedPhant->y.size() -1;
    selectedPhant->nz = selectedPhant->z.size() -1;

    updateProgress(increment);
    if (phant_file.endsWith(".egsphant")) {
        phant_file.chop(9);
        phant_file.append("_modified.egsphant");
    }
    else {
        phant_file = phant_file + "_modified.egsphant";
    }

    updateProgress(increment);
    selectedPhant->saveEGSPhantFile(phant_file);
    QProcess *qzip = new QProcess();
    qzip->setProcessChannelMode(QProcess::MergedChannels);
    qzip->setReadChannel(QProcess::StandardOutput);
    QString command = "gzip -f " + phant_file;  //-f used if exisitng .gz file, -f forces the
    qzip->execute(command);

    updateProgress(increment);
    std::cout << "Egsphant has been trimmed (dimensions x: [" << selectedPhant->x[0] << "," << selectedPhant->x[selectedPhant->nx] << "], y:["
              << selectedPhant->y[0] << "," << selectedPhant->y[selectedPhant->ny] << "], z:["
              << selectedPhant->z[0] << "," << selectedPhant->z[selectedPhant->nz] << "]) \n";
    std::cout << "Trimmed egsphant file successfully output." <<phant_file.toStdString() <<".gz  \n";

    progress->setValue(1000000000);
    progWin->hide();
    progWin->close();

    QString file = phant_file + ".gz";
    return file;
}

/***
Function: change_file_location
------------------------------
Process: Allows the user to update the location where the egsphant files are saved

***/
void Interface::change_file_location() {

    //this->setDisabled(true);
    select_location->save_info_startup();

    select_location->window->show();
    QApplication::processEvents();
    select_location->setEnabled(TRUE);

}

/***
Function: read_TAS_file
------------------------------
Input: QString with the file path

Output: Returns true if the file was read, fals if it could not be read

Process: Parses the TAS and saves the data
***/
bool Interface::read_TAS_file(QString file_path, bool containsExternal) {
    tas_contains_default = false;
    tas_contains_default_ext = false;
    tas_contains_default_int = false;

    int line = 0;
    QString tempS = "";

    QFile *file = new QFile(file_path);
    if (file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream input(file);

        while (!input.atEnd()) {
            tempS = input.readLine();
            line++;
            if (tempS.contains(" ")) {
                if (tempS.count(" ") < 2) {
                    std::cout << "Attempted to read " << file_path.toStdString() << ". File does not comply with the specified format of the Tissue Assignment Scheme file. Occurred when reading line " <<line <<".\n";
                    return false;
                }
                structure << tempS.split(' ', QString::SkipEmptyParts)[0].trimmed().replace(QString("_"), QString(" ")); //.toLower();
                tissue << tempS.split(' ', QString::SkipEmptyParts)[1];
                density_max << tempS.split(' ', QString::SkipEmptyParts)[2].toDouble();
            }
            else if (tempS.contains("\t")) {
                if (tempS.count("\t") < 2) {
                    std::cout << "Attempted to read " << file_path.toStdString() << ". File does not comply with the specified format of the Tissue Assignment Scheme file. Occurred when reading line " <<line <<".\n";
                    return false;
                }
                structure << tempS.split('\t', QString::SkipEmptyParts)[0].trimmed().replace(QString("_"), QString(" ")); //toLower().
                tissue << tempS.split('\t', QString::SkipEmptyParts)[1].trimmed();
                density_max << tempS.split('\t', QString::SkipEmptyParts)[2].toDouble();
            }
            else {
                std::cout << "Did not find a space or tab delimiter in " << file_path.toStdString() << ". File does not comply with the specified format of the Tissue Assignment Scheme file.\n";
                return false;
            }
        }

        std::cout <<"\nRead the tissue assignment scheme file " <<file_path.toStdString() <<"\n";


        for (int i=0; i<structure.size(); i++) {
            if (structure[i].contains("external", Qt::CaseInsensitive)) {
                tas_contains_default_ext = true;
            }
            if (structure[i].contains("internal", Qt::CaseInsensitive)) {
                tas_contains_default_int = true;
            }
            if (structure[i].toLower() == "default") {
                tas_contains_default = true;
            }
        }

        if (tg43Flag) {
            return true;
        }
        else if (containsExternal && !tas_contains_default_ext) {
            std::cout<<"The TAS (" <<file_path.toStdString() <<") does not contain the default external media assignment \n";
            return false;
        }
        else if (containsExternal && !tas_contains_default_int) {
            std::cout<<"The TAS (" <<file_path.toStdString() <<") does not contain the default internal media assignment \n";
            return false;
        }
        else if (!containsExternal && !tas_contains_default) {
            std::cout<<"The TAS (" <<file_path.toStdString() <<") does not contain the default media assignment \n";
            return false;
        }
        else {
            return true;
        }

    }
    else {
        std::cout << "Unable to read the TAS file " << file_path.toStdString() << "\n";
        return false;

    }
    delete file;

}


/***
Function: check_dicom_inputs
-----------------------------
Process: This function is used to:
            -notify the user if DICOM sturcture/plan/ct file is missing
            -verify the brachytherapy seed model
            -calls the function ot verify the TAS

***/
void Interface::check_dicom_inputs() {
    this->setDisabled(true);
    this->setDisabled(true);
    extraOptionsFlag = false;
    tg43Flag = false;
    //check if tas_list item selected or if its tg43 (tg43_simul_check)
    if (tas_list->currentRow() == -1 && !tg43_simul_check->isChecked()) {
        QMessageBox msgBox;
        msgBox.setText(tr("Please select a tissue assignment scheme or choose to run a TG-43 simulation."));
        msgBox.exec();
    }
    else if (!get_data->loadedstruct) { //If user hasn't uploaded Struct file
        QMessageBox msgBox;
        msgBox.setText(tr("Error: DICOM Directory doesn't contain a DICOM Struct File. \nPlease load a new DICOM directory that contains DICOM CT, Struct and Plan files."));
        msgBox.exec();
    }
    else if (!get_data->loadedct) { //If user hasn't uploaded CT file
        QMessageBox msgBox;
        msgBox.setText(tr("Error: DICOM Directory doesn't contain DICOM CT Files. \nPlease load a new DICOM directory that contains DICOM CT, Struct and Plan files."));
        msgBox.exec();
    }
    else if (!get_data->loadedplan) {   //if user hasn't uploaded Plan file
        QMessageBox msgBox;
        msgBox.setText(tr("Error: DICOM Directory doesn't contain a DICOM Plan File. \nPlease load a new DICOM directory that contains DICOM CT, Struct and Plan files."));
        msgBox.exec();
    }
    else {      //If all necessary dicom files are uploaded (at least one CT, one Plan and a Struct file)
        this->setDisabled(true);
        if (tg43_simul_check->isChecked()) {
            tg43Flag = true;
            TAS_file = working_path +"/Tissue_Assignment_Schemes/tg43.txt";
        }
        //User prompt to select the correct seed
        //--------------------------------------
        isotope = get_data->isotope_name;
        privateSeedInfo = get_data->Seed_info;
        bool continue_function = get_seed_from_user();

        if (continue_function == false) { //user has pressed cancel in seed selection
            this->setEnabled(true);
            return;
        }

        //multiply by 100 because air kerma per hist is cm2, we want mm2
        DSF = get_data->DSF_intermediate/(air_kerma_per_history*100);

        // Tissue Assignment Scheme
        //---------------------------
        if (!tg43Flag) {
            TAS_file = tas_names_path[tas_list->currentRow()];
        }

        structure.clear();
        tissue.clear();
        density_max.clear();
        structUnique.clear();

        if (tg43Flag == true) {

            //If unable to read the tg43 fle, use the default
            if (!read_TAS_file(TAS_file, false)) {
                structure << "Water";
                tissue << "WATER_0.998";
                density_max << 100;
                std::cout<<"Using default TG-43 media " <<tissue.last().toStdString() <<"\n";
            }

            structUnique.append(false);
            setup_egsphant();

        }
        else {

            while (!read_TAS_file(TAS_file, get_data->externalStruct)) {
                QMessageBox msgBox;
                msgBox.setText(tr("Please select a valid tissue assignment scheme (.txt) file."));
                msgBox.setWindowTitle(tr("egs_brachy GUI"));
                msgBox.exec();

                TAS_file = QFileDialog::getOpenFileName(
                               this,
                               tr("Select a valid Tissue Assignment Scheme"),
                               working_path + "/Tissue_Assignment_Schemes/",
                               "Text (*.txt)");

                if (TAS_file.isEmpty()) { //user clicked cancel
                    return;
                }
            }


            //Ask user to verify the tissue assignment scheme
            QVector<QString> unique_struct;
            for (int i=0; i<structure.size(); i++) {
                if (!unique_struct.contains(structure[i])) {
                    unique_struct.append(structure[i]);
                }
            }

            tissue_check = new Tissue_check(this, get_data->structName, unique_struct, get_data->structType);

            //tissue_check->setDisabled(TRUE);
            tissue_check->window->show();
            QApplication::processEvents();
            tissue_check->setEnabled(TRUE);

            connect(tissue_check, SIGNAL(closed()), this, SLOT(set_tissue_selection()));
            connect(tissue_check, SIGNAL(exit()), this, SLOT(stop_process()));

            tissue_check->setDisabled(TRUE);
            QApplication::processEvents();


        }
    }



    this->setEnabled(true);
}

/***
Function::stop_process
----------------------
Process: used to stop the current process running
***/
void Interface::stop_process() {
    this->setEnabled(true);
    return;
}

void Interface::set_tissue_selection() {
    //External media was removed in the TAS
    QVector<QString> tas(get_data->structName.size());
    genMask.resize(get_data->structName.size());
    genMask.fill(false);
    int j=0;
    for (int i=0; i<get_data->structName.size(); i++) {
        if (get_data->structType[i] == "EXTERNAL") {
            tas[i] = " ";
        }
        else {
            tas[i] = tissue_check->selected_tas_name[j];
            genMask[i] = tissue_check->generateMask[j];
            j++;
        }
    }

    structUnique.resize(get_data->structName.size());
    external.resize(get_data->structName.size());
    external.fill(false);
    structPrio.resize(get_data->structName.size());

    //empty all vectors
    medThreshold.clear();
    denThreshold.clear();
    medThresholds.clear();
    denThresholds.clear();
    medThreshold_int.clear();
    denThreshold_int.clear();

    //Itterate through the structures, finding the default assignment
    for (int i=0; i<structure.size(); i++) {
        if (get_data->externalStruct) {
            if (structure[i].contains("external", Qt::CaseInsensitive)) {
                medThreshold << tissue[i];
                denThreshold << density_max[i];
            }
            else if (structure[i].contains("internal", Qt::CaseInsensitive)) {
                medThreshold_int << tissue[i];
                denThreshold_int << density_max[i];
            }
        }
        else if (!get_data->externalStruct) {
            if (structure[i].toLower() == "default") {
                medThreshold << tissue[i];
                denThreshold << density_max[i];
            }
        }
    }

    //Itterate through the structures, matching the TAS structure name to the name in the DICOM files
    if (get_data->externalStruct) {
        for (int i=0; i<tas.size(); i++) {
            bool foundStruct = false;
            medThresholds.resize(medThresholds.size()+1);
            denThresholds.resize(denThresholds.size()+1);

            if (tas[i] == " " || tas[i].contains("Default",Qt::CaseInsensitive)) { //The dicom structure was unassigned
                medThresholds.last() = medThreshold_int;
                denThresholds.last() = denThreshold_int;
                foundStruct = true;
                structUnique[i] = false;
            }
            else {
                for (int j=0; j<structure.size(); j++)
                    if (tas[i] ==  structure[j]) {
                        medThresholds.last() << tissue[j];
                        denThresholds.last() << density_max[j];
                        foundStruct = true;
                        structUnique[i] = true;
                    }
            }
            if (!foundStruct) {
                std::cout << "Did not find " << get_data->structName[i].toStdString() << " in the tissue assignment scheme file, assuming default media assignment.\n";
                medThresholds.last() = medThreshold_int;
                denThresholds.last() = denThreshold_int;
                structUnique[i] = false;
            }
            //std::cout<<get_data->structName[i].toStdString() <<" " <<structUnique[i] <<"\n";
        }
    }
    else {
        for (int i=0; i<tas.size(); i++) {
            bool foundStruct = false;
            medThresholds.resize(medThresholds.size()+1);
            denThresholds.resize(denThresholds.size()+1);

            if (tas[i] == " " || tas[i].toLower() == "default") { //contains("Default",Qt::CaseInsensitive)){ //The dicom structure was unassigned
                medThresholds.last() = medThreshold;
                denThresholds.last() = denThreshold;
                foundStruct = true;
                structUnique[i] = false;
            }
            else {
                for (int j=0; j<structure.size(); j++)
                    if (tas[i] ==  structure[j]) {
                        medThresholds.last() << tissue[j];
                        denThresholds.last() << density_max[j];
                        foundStruct = true;
                        structUnique[i] = true;
                    }
            }
            if (!foundStruct) {
                medThresholds.last() = medThreshold;
                denThresholds.last() = denThreshold;
                structUnique[i] = false;
            }
            //std::cout<<get_data->structName[i].toStdString() <<" " <<structUnique[i] <<"\n";
        }
    }
    setup_egsphant();

}

void Interface::setup_egsphant() {

    if (!readCalibFile) { //Typically here if reading the default file
        QString fileName;
        if (!readCalib()) {
            do {
                QMessageBox msgBox;
                msgBox.setText(tr("The CT calibration file could not be read. Please select a valid hu2rho file.\nThe user manual contains the hu2rho file specifications."));
                msgBox.setWindowTitle(tr("egs_brachy GUI"));
                msgBox.exec();

                fileName = QFileDialog::getOpenFileName(
                               this,
                               tr("Select a valid CT calibration file"),
                               working_path, "hu2rho (*.hu2rho)");

                if (fileName.isEmpty() || fileName.isNull()) {
                    return;
                }
                else {
                    density = fileName;
                    readCalib();
                }

            }
            while (!readCalibFile);
        }

        if (readCalibFile) {
            std::cout<<"Successfully read CT calibration file " <<density.toStdString() <<"\n";
        }
    }

    // ---------------------------------------------------------- //
    // SETUP EGSPHANT AND HU CONVERSION MAPS/THRESHOLDS           //
    // ---------------------------------------------------------- //


    this->setDisabled(true);
    if (get_data->treatment_type == "LDR" || get_data->treatment_type == "MANUAL") {
        if (structUnique.contains(true)) {
            get_prio = new priority(this, get_data->structName, structUnique);

            get_prio->window->show();
            QApplication::processEvents();
            get_prio->setEnabled(TRUE);

            connect(get_prio, SIGNAL(closed()), this, SLOT(setup_MAR()));
            connect(get_prio, SIGNAL(exit()), this, SLOT(stop_process()));

            get_prio->setDisabled(TRUE);
            QApplication::processEvents();

        }
        else {
            setup_MAR();
        }
    }
    else {
        if (structUnique.contains(true)) {
            get_prio = new priority(this, get_data->structName, structUnique);

            get_prio->window->show();
            QApplication::processEvents();
            get_prio->setEnabled(TRUE);

            if (extraOptionsFlag) {
                connect(get_prio, SIGNAL(closed()), this, SLOT(ATcreate_egsphant()));
                connect(get_prio, SIGNAL(exit()), this, SLOT(stop_process()));
            }
            else {
                connect(get_prio, SIGNAL(closed()), this, SLOT(create_egsphant()));
                connect(get_prio, SIGNAL(exit()), this, SLOT(stop_process()));
            }

            get_prio->setDisabled(TRUE);
            QApplication::processEvents();

        }
        else {
            if (extraOptionsFlag) {
                ATcreate_egsphant();
            }
            else {
                create_egsphant();
            }
        }
    }
    this->setEnabled(true);
}

/***
Function::create_egsphant
----------------------
Process: Creates the egsinp files
***/
void Interface::create_egsphant() {
    this->setDisabled(true);

    if (structUnique.contains(true)) {
        structPrio = get_prio->structPrio;
    }
    // ---------------------------------------------------------- //
    // GENERATE EGSPHANT AND HU CONVERSION MAPS/THRESHOLDS        //
    // ---------------------------------------------------------- //
    std::clock_t start;
    double duration;
    start = std::clock();


    // ---------------------------------------------------------- //
    // SETUP THE STRUCTURES AND MEDIA                             //
    // ---------------------------------------------------------- //
    QMap <QString,unsigned char> mediaMap;
    phant.media.clear();

    if (tg43Flag) {

        // Track the number of media, and create a lookup for media to ASCII character representation
        int medNum = 0;
        phant.media.append(tissue[0]);
        mediaMap.insert(tissue[0], 49 + medNum + (medNum>8?7:0) + (medNum>34?6:0));



        structUnique.resize(get_data->structName.size());
        structUnique.fill(false);
        structPrio.resize(get_data->structName.size());
        structPrio.fill(0);
        genMask.resize(get_data->structName.size());
        genMask.fill(false);
        external.resize(get_data->structName.size());
        external.fill(false);

        if (indexMARContour != -1) {
            structUnique[indexMARContour] = true;
            structPrio[indexMARContour] = 1;
        }

    }
    else {

        //Assign enternal contours a default priority
        for (int i = 0; i < get_data->structName.size(); i++) {
            if (get_data->structType[i] ==  "EXTERNAL") {
                structUnique[i] = true;
                structPrio[i] = 1;
                external[i] = true;
            }
        }

        // Track the number of media, and create a lookup for media to ASCII character representation
        int medNum = 0;

        for (int i = 0; i < medThreshold.size(); i++) {
            if (!mediaMap.contains(medThreshold[i])) {
                phant.media.append(medThreshold[i]);
                mediaMap.insert(medThreshold[i], 49 + medNum + (medNum>8?7:0) + (medNum>34?6:0));
                medNum++; // 49 is the ASCII value of '1', 7 jumps from ';' to 'A', 6 jumps from '[' to 'a'
            }
        }

        for (int i = 0; i < get_data->structName.size(); i++) {
            for (int j = 0; j < medThresholds[i].size(); j++)
                if (!mediaMap.contains(medThresholds[i][j])) {
                    phant.media.append(medThresholds[i][j]);
                    mediaMap.insert(medThresholds[i][j], 49 + medNum + (medNum>8?7:0) + (medNum>34?6:0));
                    medNum++; // 49 is the ASCII value of '1', 7 jumps from ';' to 'A', 6 jumps from '[' to 'a'
                }
        }

    }
    // ---------------------------------------------------------- //
    // CONVERTING HU TO APPROPRIATE DENSITY AND MEDIUM            //
    // ---------------------------------------------------------- //
    setup_progress_bar("Generating the phantom", "");

    double increment =  1000000000/(phant.nx*phant.nz +3);

    updateProgress(increment);
    QVector<QString> maskName;
    QMap <int, int> maskToStructMap;
    media = phant.media;


    // Get bounding rectangles over each struct
    QVector <QVector <QRectF> > structRect;
    for (int i = 0; i < get_data->structPos.size(); i++) {
        structRect.resize(i+1);
        for (int j = 0; j < get_data->structPos[i].size(); j++) {
            structRect[i].resize(j+1);
            structRect[i][j] = get_data->structPos[i][j].boundingRect();
        }
    }


    updateProgress(increment);
    int j=0;
    // Setup masks
    if (!get_data->structName.isEmpty()) {
        for (int i = 0; i < get_data->structName.size(); i++) {
            if (genMask[i]) {
                EGSPhant *temp = new EGSPhant;
                temp->makeMask(&phant);
                maskName.append(get_data->structName[i]);
                masks << temp;
                maskToStructMap.insert(i,j); //i == key, j == value
                j++;
            }
        }
    }

    updateProgress(increment);
    // Arrays that hold the struct numbers and center voxel values to be used
    QList<QPoint> zIndex, yIndex, zExtIndex, yExtIndex;
    QList<QPoint>::iterator p;
    QVector<int> zSliceNoStruct;
    double zMid, yMid, xMid;
    int tempHU = 0, n = 0, q = 0, inStruct = 0, prio = 0;

    // Convert HU to density and media without masks
    for (int k = 0; k < phant.nz; k++) { // Z //
        if (get_data->structZ.size() > 0) {
            zIndex.clear(); // Reset lookup
            zMid = (phant.z[k]+phant.z[k+1])/2.0;
            for (int l = 0; l < get_data->structZ.size(); l++)
                if (structUnique[l])
                    for (int m = 0; m < get_data->structZ[l].size(); m++) {
                        // If slice j of struct i on the same plane as slice k of the phantom
                        if (abs(get_data->structZ[l][m] - zMid) < (phant.z[k+1]-phant.z[k])/2.0) {
                            zIndex << QPoint(l,m);    // Add it to lookup
                        }
                    }
            //if(zIndex.size() == 0 && !tg43Flag) //slice has no contour data
            //zSliceNoStruct.append(k);

        }

        for (int j = 0; j < phant.ny; j++) { // Y //
            if (zIndex.size() > 0) {
                yIndex.clear(); // Reset lookup
                yMid = (phant.y[j]+phant.y[j+1])/2.0;
                for (p = zIndex.begin(); p != zIndex.end(); p++) {
                    // If column p->y() of struct p->x() on the same column as slice k,j of the phantom
                    if (structRect[p->x()][p->y()].top() <= yMid && yMid <= structRect[p->x()][p->y()].bottom()) {
                        yIndex << *p;
                    }
                }
            }

            for (int i = 0; i < phant.nx; i++) { // X //
                tempHU = get_data->HU[k][j][i];
                xMid = (phant.x[i]+phant.x[i+1])/2.0;

                // Linear search because I don't think these arrays every get big
                // get the right density
                for (n = 0; n < HUMap.size()-1; n++)
                    if (HUMap[n] <= tempHU && tempHU < HUMap[n+1]) {
                        break;
                    }
                if (tempHU < HUMap[0]) {
                    n = 0;
                }
                if (tempHU > HUMap[HUMap.size()-1]) {
                    n = HUMap.size() -2;
                }


                double temp = interp(tempHU,HUMap[n],HUMap[n+1],denMap[n],denMap[n+1]);
                temp = temp<=0?0.000001:temp; // Set min density to 0.000001

                if (temp > phant.maxDensity) { // Track max density for images
                    phant.maxDensity = temp;
                }

                //if in the selected contour, replace with low threshold value
                if (phant.d[i][j][k] == 0) {
                    phant.d[i][j][k] = temp; //assign density
                }
                else {
                    temp = phant.d[i][j][k];
                }

                // get the right media
                if (yIndex.size() > 0) {
                    inStruct = 0;
                    prio = 0;
                    for (p = yIndex.begin(); p != yIndex.end(); p++) { // Check through each
                        // If row p->y() of struct p->x() on the same row as slice k,j,i of the phantom
                        if (structRect[p->x()][p->y()].left() <= xMid && xMid <= structRect[p->x()][p->y()].right())
                            if (get_data->structPos[p->x()][p->y()].containsPoint(QPointF(xMid,yMid), Qt::OddEvenFill)) {
                                if (structPrio[p->x()] > prio) {
                                    inStruct = p->x()+1; // Inflate index for the next check
                                    prio = structPrio[p->x()]; // Set priority at this struct's prio
                                }
                            }
                    }
                }

                if (inStruct) { // Deflate index again
                    inStruct--;

                    q = get_data->structLookup[get_data->structReference[inStruct]]; // get the structName index which matches denThresholds

                    //Flag if in the selected contour for STR
                    if (setup_MAR_Flag && q == indexMARContour && phant.d[i][j][k] < low_threshold) {
                        phant.d[i][j][k] = replacement;
                    }
                    //Find the media of the voxel
                    if (!tg43Flag) {
                        for (n = 0; n < denThresholds[q].size()-1; n++)
                            if (temp < denThresholds[q][n]) {
                                break;
                            }

                        phant.m[i][j][k] = mediaMap[medThresholds[q][n]];
                        phant.contour[i][j][k] = q;

                        //To generate mask
                        if (genMask[inStruct]) {
                            if (maskToStructMap.contains(inStruct)) {
                                int idx = maskToStructMap.value(inStruct);
                                masks[idx]->m[i][j][k] = 1;
                            }
                        }
                    }
                    else if (tg43Flag) {
                        phant.m[i][j][k] = 49;
                    }
                }
                else {
                    if (!tg43Flag) {
                        for (n = 0; n < denThreshold.size()-1; n++)
                            if (temp < denThreshold[n]) {
                                break;
                            }
                        phant.m[i][j][k] = 49 + n + (n>8?7:0) + (n>34?6:0);
                    }
                    else if (tg43Flag) {
                        phant.m[i][j][k] = 49;
                    }
                }
            }
            updateProgress(increment);
        }
    }

    //Interpolate the struct data using slice of previous index
    if (!zSliceNoStruct.isEmpty() && !tg43Flag) {
        std::cout<<"Interpolating the struct data \n";
        //Copy the media of the previous slice
        for (int m=0; m<zSliceNoStruct.size(); m++)
            if (zSliceNoStruct[m] == 0) { //special case - can't copy previous slice, copy the next filled slice
                bool foundNextFilled = false;
                int nextSlice = 0;
                while (!foundNextFilled) {
                    nextSlice++;
                    if (!zSliceNoStruct.contains(nextSlice)) {
                        foundNextFilled = true;
                    }
                }

                for (int j = 0; j < phant.ny; j++)
                    for (int i = 0; i < phant.nx; i++) {
                        phant.contour[i][j][0] = phant.contour[i][j][nextSlice];
                        for (int idx = 0; idx<masks.size(); idx++) {
                            masks[idx]->m[i][j][0] = masks[idx]->m[i][j][nextSlice];
                        }

                        if (phant.contour[i][j][0] != 0) {

                            if (setup_MAR_Flag && phant.contour[i][j][0] == indexMARContour && phant.d[i][j][0] < low_threshold) { //STR
                                phant.d[i][j][0] = replacement;
                            }

                            if (!tg43Flag) {
                                for (n = 0; n < denThresholds[phant.contour[i][j][0]].size()-1; n++)
                                    if (phant.d[i][j][0] < denThresholds[phant.contour[i][j][0]][n]) {
                                        break;
                                    }

                                phant.m[i][j][0] = mediaMap[medThresholds[phant.contour[i][j][0]][n]];

                            }
                            else {
                                phant.m[i][j][0] = 49;
                            }

                        }
                        else {
                            if (!tg43Flag) {
                                for (n = 0; n < denThreshold.size()-1; n++)
                                    if (phant.d[i][j][0] < denThreshold[n]) {
                                        break;
                                    }

                                phant.m[i][j][0] = 49 + n + (n>8?7:0) + (n>34?6:0);
                            }
                            else {
                                phant.m[i][j][0] = 49;
                            }
                        }

                    }
            }
            else {
                std::cout<<zSliceNoStruct[m] <<"\n";
                int prevSlice = zSliceNoStruct[m]-1;
                for (int j = 0; j < phant.ny; j++)
                    for (int i = 0; i < phant.nx; i++) {
                        phant.contour[i][j][zSliceNoStruct[m]] = phant.contour[i][j][prevSlice];
                        for (int idx = 0; idx<masks.size(); idx++) {
                            masks[idx]->m[i][j][zSliceNoStruct[m]] = masks[idx]->m[i][j][prevSlice];
                        }

                        if (phant.contour[i][j][zSliceNoStruct[m]] != 0) {

                            if (setup_MAR_Flag && phant.contour[i][j][zSliceNoStruct[m]] == indexMARContour && phant.d[i][j][zSliceNoStruct[m]] < low_threshold) { //STR
                                phant.d[i][j][zSliceNoStruct[m]] = replacement;
                            }

                            if (!tg43Flag) {
                                for (n = 0; n < denThresholds[phant.contour[i][j][zSliceNoStruct[m]]].size()-1; n++)
                                    if (phant.d[i][j][zSliceNoStruct[m]] < denThresholds[phant.contour[i][j][zSliceNoStruct[m]]][n]) {
                                        break;
                                    }

                                phant.m[i][j][zSliceNoStruct[m]] = mediaMap[medThresholds[phant.contour[i][j][zSliceNoStruct[m]]][n]];

                            }
                            else {
                                phant.m[i][j][zSliceNoStruct[m]] = 49;
                            }

                        }
                        else {
                            if (!tg43Flag) {
                                for (n = 0; n < denThreshold.size()-1; n++)
                                    if (phant.d[i][j][zSliceNoStruct[m]]  < denThreshold[n]) {
                                        break;
                                    }

                                phant.m[i][j][zSliceNoStruct[m]] = 49 + n + (n>8?7:0) + (n>34?6:0);
                            }
                            else {
                                phant.m[i][j][zSliceNoStruct[m]] = 49;
                            }
                        }
                    }
            }
    }

    //}
    duration = (std::clock()-start)/(double)CLOCKS_PER_SEC;
    std::cout << "Successfully generated egsphant (dimensions x: [" << phant.x[0] << "," << phant.x[phant.nx] << "], y:["
              << phant.y[0] << "," << phant.y[phant.ny] << "], z:["
              << phant.z[0] << "," << phant.z[phant.nz] << "]).  Time elapsed is " << duration << " s.\n";


    progress->setValue(1000000000);
    progWin->hide();
    progWin->close();


    // ---------------------------------------------------------- //
    // OUTPUT IMAGES                                              //
    // ---------------------------------------------------------- //

    // Output and delete masks
    if (!tg43Flag) {
        if (!get_data->structName.isEmpty() && !masks.isEmpty()) {
            increment =     1000000000/(2*masks.size() +1);
            setup_progress_bar("Generating the structure masks", "");
            updateProgress(increment);

            for (int i = masks.size()-1; i >= 0; i--) {
                updateProgress(increment);
                masks[i]->saveEGSPhantFile(maskName[i]+"_mask.egsphant");
                updateProgress(increment);
                delete masks[i];
            }
            masks.clear();
            duration = (std::clock()-start)/(double)CLOCKS_PER_SEC;
            std::cout << "Masks successfully output.  Time elapsed is " << duration << " s.\n";
        }

        if (tissue_check->generateImages) {
            increment = 1000000000/(2*phant.nz +3);
            setup_progress_bar("Generating the images", "");
            updateProgress(increment);

            std::cout<<"Generating image data \n";
            double xi = (phant.x[0]+phant.x[1])/2.0;
            double xf = (phant.x[phant.nx-1]+phant.x[phant.nx])/2.0;
            double yi = (phant.y[0]+phant.y[1])/2.0;
            double yf = (phant.y[phant.ny-1]+phant.y[phant.ny])/2.0;
            double z;
            updateProgress(increment);
            double res = 10.0/get_data->xySpacing[0][0]*2; // This sets resolution to be 2 pixels for each voxel in x
            QImage temp;
            QList <QPointF> tempF;
            QList<QPointF>::iterator tempIt;
            QPen pen;
            double zMid;
            pen.setWidth(2);
            updateProgress(increment);
            for (int i = 0; i < phant.nz; i++) {
                z = get_data->imagePos[i][2]/10.0;
                phant.getEGSPhantPicDen("z axis", yi, yf, xi, xf, z, res).save(QString("Image/DenPic")+QString::number(i+1)+".png");

                temp = phant.getEGSPhantPicMed("z axis", yi, yf, xi, xf, z, res);
                updateProgress(increment);
                QPainter paint(&temp);
                for (int j = 0; j < get_data->structZ.size(); j++)
                    for (int k = 0; k < get_data->structZ[j].size(); k++) {
                        zMid = (phant.z[i]+phant.z[i+1])/2.0;
                        pen.setColor(QColor(double(j)/double(get_data->structZ.size())*255.0,0,255.0-double(j)/double(get_data->structZ.size())*255.0));
                        paint.setPen(pen);
                        if (abs(get_data->structZ[j][k] - zMid) < (phant.z[i+1]-phant.z[i])/2.0) {
                            tempF = get_data->structPos[j][k].toList();
                            for (tempIt = tempF.begin(); tempIt != tempF.end(); tempIt++) {
                                paint.drawPoint(phant.getIndex("x axis", tempIt->x())*2, phant.getIndex("y axis", tempIt->y())*2);
                            }
                        }
                    }
                temp.save(QString("Image/MedPic")+QString::number(i+1)+".png");
                updateProgress(increment);
            }

            progress->setValue(1000000000);
            progWin->hide();
            progWin->close();


            duration = (std::clock()-start)/(double)CLOCKS_PER_SEC;
            std::cout << "Image data successfully output.  Time elapsed is " << duration << " s.\n";
        }

    }

    //-----------------------------------------------------------------
    // Save egsphant file
    //-----------------------------------------------------------------
    if (select_location->changed_location == true) { //user has changed the egsphant file location
        egs_input->egsphant_location = select_location->egsphant_path->text();
    }

    //std::cout<<"Saving the egsphant file as " <<egs_input->egsphant_location.toStdString() <<"\n";

    //Fix: change this file save location
    phant.saveEGSPhantFile(egs_input->egsphant_location);

    //std::cout<<"Reducing file size of the egsphant (using gzip) \n";

    QProcess *qzip = new QProcess();
    qzip->setProcessChannelMode(QProcess::MergedChannels);
    qzip->setReadChannel(QProcess::StandardOutput);

    //Actual line of exectuion
    QString command = "gzip -f " + egs_input->egsphant_location;  //-f used if exisitng .gz file, -f forces the
    //file to be created
    qzip->execute(command);

    //phant.savebEGSPhantFile("PrimaryOutput.begsphant");
    duration = (std::clock()-start)/(double)CLOCKS_PER_SEC;
    std::cout << "Egsphant file successfully output." <<egs_input->egsphant_location.toStdString() <<".gz  Time elapsed is " << duration << " s.\n";

    trimEGS->generatedPhant = true;
    preview = new Preview(this, mediaMap);

    create_egsinp_files();

}


/***
Function: create_egsinp_files
-----------------------------
Process: create the egs_brachy input files (egsphant, egsinp and seed transformations)
         Reads the spectra and muen files, if the files can't be found, user is asked to select the file
***/
void Interface::create_egsinp_files() {

    if (select_location->changed_location == true) { //user has changed the egsinp file location

        egs_input->seed_file = select_location->transformation_path->text();
        egs_input->egsinp_path = select_location->egsinp_path->text();
        //egs_input->egsphant_location = select_location->egsphant_path->text();
    }

    //Getting the egsinp file name (without the filename extension)
    int n = egs_input->egsinp_path.count("/");
    QString egsinp_folder_name = egs_input->egsinp_path.section("/", n, n);
    int period = egsinp_folder_name.lastIndexOf(".");
    egs_input->egsinp_name = egsinp_folder_name.left(period);


    egs_input->create_transformation_file(get_data->all_seed_pos, get_data->air_kerma);

    QVector<double> source_weights;
    if (get_data->treatment_type == "HDR") {
        double max_weight = 0;
        for (int i=0; i<get_data->all_seed_time.size(); i++) {
            for (int j=0; j<get_data->all_seed_time[i].size(); j++) {
                source_weights.append(round((get_data->all_seed_time[i][j]/get_data->total_time)*10000.0)/10000.0);
                if (source_weights.last() > max_weight) {
                    max_weight = source_weights.last();
                }
            }
        }
        DSF = DSF*max_weight;
    }

    egs_input->boxFilePath = trimEGS->inscribedPhantPath;
    egs_input->applicatorFilePath = trimEGS->applicatorPath;

    //Creating and outputting the egsinp
    //----------------------------------
    egs_input->create_egsinp(element, seed, DSF, get_data->treatment_type,
                             source_weights, phant.media, tg43Flag,
                             options->checked_score_energy_deposition, options->muen_file, options->number_hist,  options->number_batch,
                             options->number_chunk, select_location->egsinp_header, options->transport_param_path, options->material_file);


    //Enable the preview and launch egs_brachy buttons
    PreviewButton->setToolTip(tr("Preview the phantom slice-by-slice"));
    launchButton->setToolTip(tr("Launch egs_brachy and view metrics"));
    launchButton->setEnabled(1);
    PreviewButton->setEnabled(1);




    progress->setValue(1000000000);

    progWin->hide();
    progWin->lower();
    this->setEnabled(true); //Re-enable the main window
}



/***
Function: show_preview
-----------------------
Process: opens the phantom previewer window
***/
void Interface::show_preview() {
    //preview = new Preview(this, mediaMap);
    preview->phant.nx = phant.nx;
    preview->phant.nz = phant.nz;
    preview->phant.ny = phant.ny;
    preview->phant.x = phant.x;
    preview->phant.z = phant.z;
    preview->phant.y = phant.y;
    preview->phant.m = phant.m;
    preview->phant.d = phant.d;
    preview->phant.contour = phant.contour;
    preview->phant.media = phant.media;
    preview->phant.maxDensity = phant.maxDensity;

    preview->createwindow();
    //preview->setDisabled(TRUE);

    //this->setDisabled(true);
    preview->window->show();
    QApplication::processEvents();
    preview->setEnabled(TRUE);
    preview->redraw();

    connect(preview, SIGNAL(closed()), this, SLOT(delete_preview()));

}


/***
Function: delete_preview
-----------------------
Process: disables the phantom previewer window
***/
void Interface::delete_preview() {
    preview->setDisabled(TRUE);
    QApplication::processEvents();
}





/***
Function: get_seed_from_user
----------------------------

Process: A series of pop-up windows that asks the user to either:
            -verify the Source found in the dicom data (change if not correct)
            -select a seed based on the models available in egs_brachy (seed couldn't be found in dicom)
Output: bool that returns false if the user clicks cancel (wants to stop)
***/
bool Interface::get_seed_from_user() {

    QStringList seeds = get_seedList().toList();
    bool ok;
    bool ok_airkerma;
    bool got_air_kerma = false;
    this->setDisabled(true);
    QString item;

    if (PredictedSeedIndex ==0) { //Didn't find seed in the DICOM file
        item = QInputDialog::getItem(this, tr("Select the Source Model"),
                                     tr("Select the correct source model:                   "),
                                     seeds, 0, false, &ok);
    }
    else {
        item = QInputDialog::getItem(this, tr("Select the Source Model"),
                                     tr("Verify the predicted source model:                  "),
                                     seeds,PredictedSeedIndex, false, &ok);


    }
    this->setEnabled(true);

    //When user selected seed and closed window
    if (ok && !item.isEmpty()) {
        //Get the air kerma per history
        for (int i=0; i<seeds.size(); i++) {
            if (seeds[i] == item) {
                element = source_element_folder_names[i];
                seed = item;
            }
        }

        QString filename =  "Air_kerma/" + item + ".Sk" ;
        //std::cout<<filename.toStdString();

        QFile *file = new QFile(filename);
        if (file->open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream input(file);

            while (!input.atEnd()) {
                air_kerma_per_history = input.readLine().toDouble(&ok_airkerma);
            }

            if (!ok_airkerma) {
                std::cout<<"Unable to read the contents of the selected air kerma per history (.Sk) file " <<filename.toStdString() <<"\n";
                got_air_kerma = select_air_kerma(item);
            }
            else {
                got_air_kerma = true;
            }

        }
        else {   //Not a valid file
            std::cout<<"Warning: Unable to read the air kerma per history (.Sk) file. Searched for " <<filename.toStdString() <<"\n";
            got_air_kerma = select_air_kerma(item);
        }
        delete file;
    }

    if (!ok || !got_air_kerma) {
        return false;
    }


    return true;
}

// Creates file dialog for user to select the air kerma per hist file
bool Interface::select_air_kerma(QString seed) {

    bool got_air_kerma = false;
    bool ok_airkerma;
    this->setDisabled(true);

    while (!got_air_kerma) {

        QMessageBox msgBox;
        msgBox.setText(tr("The air kerma per history (.Sk) file for the ") + seed + tr(" source could not be found. \nPlease select the appropriate .Sk file."));
        msgBox.setWindowTitle(tr("egs_brachy GUI"));
        msgBox.exec();

        QString air_kerma_file = QFileDialog::getOpenFileName(
                                     this,
                                     tr("Select the air kerma per history file corresponding to the source"),
                                     working_path + "/Air_kerma/",
                                     "Air kerma (*.Sk)");

        if (air_kerma_file.isEmpty()) { //user clicked cancel
            this->setEnabled(true);
            return false;
        }

        QFile *file2 = new QFile(air_kerma_file);
        if (file2->open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream input2(file2);

            while (!input2.atEnd()) {
                air_kerma_per_history = input2.readLine().toDouble(&ok_airkerma);
            }

            if (!ok_airkerma) {
                std::cout<<"Unable to read the contents of the selected air kerma per history (.Sk) file " <<air_kerma_file.toStdString() <<"\n";
                got_air_kerma = false;
            }
            else {
                got_air_kerma = true;
            }


        }
        else {
            std::cout<<"Unable to read the selected air kerma per history (.Sk) file " <<air_kerma_file.toStdString() <<"\n";
            got_air_kerma = false;
        }

        delete file2;

    }
    this->setEnabled(true);
    return true;
}
/***
Function: get_seedList
----------------------
Process: uses the path of the egs_brachy sources to get a vector of the possible sources
***/
QVector<QString> Interface::get_seedList() {
    QVector<QString> file_path; //seed name

    QString sources_path = source_folder_path + "/";

    QDir dir(sources_path);
    QFileInfoList fileNames = dir.entryInfoList();
    QDirIterator directories(sources_path, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    //Reads all the files in sub-directories of the dir_path
    while (directories.hasNext()) { //one more entry in the directory
        QFile f(directories.next());    //advances iterator to next entry
        QDir dir1(f.fileName());
        QFileInfoList fileNam = dir1.entryInfoList();   //all files and directories in the sub-directory

        for (int i=0; i < fileNam.size(); i ++) { //itterating through all the files
            //in the sub-directory
            QFileInfo fileInf = fileNam.at(i);

            if (fileInf.isFile()) { //Adds the file to the list of file names
                fileNames.push_back(fileInf);
            }

        }
    }

    if (!fileNames.isEmpty() && !sources_path.isEmpty()) {
        for (int i=0; i < fileNames.size(); i ++) {

            QFileInfo fileInfo = fileNames.at(i);

            if (fileInfo.isFile() && !fileInfo.absoluteFilePath().contains("README")) {
                //Getting the name of seed
                QString abs_path = fileInfo.absoluteFilePath();
                int idx = abs_path.lastIndexOf("/");
                abs_path = abs_path.left(idx);
                idx = abs_path.lastIndexOf("/")+1;

                //Getting the element name
                int idx_elem = abs_path.lastIndexOf("/");
                QString element = abs_path.left(idx_elem);
                idx_elem = element.lastIndexOf("/")+1;

                if (!file_path.contains(abs_path.right(abs_path.length()-idx))) {
                    file_path.append(abs_path.right(abs_path.length()-idx)); //seed
                    source_element_folder_names.append(element.right(element.length()-idx_elem));

                }
            }
        }
    }

    //Try to predict the seed using the seed info
    QVector <int> index_element_folder;
    PredictedSeedIndex = 0;
    if (!isotope.isEmpty()) {
        std::string isotopestr = isotope.toStdString();
        size_t found_num = isotopestr.find_first_of("0123456789");
        size_t found_char = isotopestr.find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
        size_t found_num_last = isotopestr.find_last_of("0123456789");
        size_t found_char_last = isotopestr.find_last_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

        QString element = isotope.mid(found_char, found_char_last+1-found_char);
        QString number = isotope.mid(found_num, found_num_last+1-found_num);

        for (int i=0; i<source_element_folder_names.size(); i++)
            if (source_element_folder_names[i].contains(element, Qt::CaseInsensitive) || source_element_folder_names[i].contains(element, Qt::CaseInsensitive)) {
                index_element_folder.append(i);
            }
    }

    if (index_element_folder.size() == 1) {
        PredictedSeedIndex = index_element_folder[0];
    }

    //If there are no or multiple element matches
    if (!privateSeedInfo.isEmpty() && index_element_folder.size() != 1) {
        if (index_element_folder.size() > 1) {
            for (int i=0; i<index_element_folder.size(); i++)
                for (int j=0; j<privateSeedInfo.size(); j++)
                    if (file_path[i].contains(privateSeedInfo[j], Qt::CaseInsensitive) || privateSeedInfo[j].contains(file_path[i], Qt::CaseInsensitive)) {
                        PredictedSeedIndex = i;
                    }

        }
        else {
            for (int i=0; i<file_path.size(); i++)
                for (int j=0; j<privateSeedInfo.size(); j++)
                    if (file_path[i].contains(privateSeedInfo[j], Qt::CaseInsensitive) || privateSeedInfo[j].contains(file_path[i], Qt::CaseInsensitive)) {
                        PredictedSeedIndex = i;
                    }
        }
    }

    return file_path;
}


/***
Function: setup_progress_bar
----------------------------
Process: This function is used to create and raise the progres bar

Inputs: window_title: the window name of the progress bar
        text: an optional label to add to the progress bar, used to describe the process running
             if text is an empty string, no lable is displayed
***/
void Interface::setup_progress_bar(QString window_title, QString text) {
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
Function: setup_progress_bar
----------------------------
Process: This function is used to create and raise the progres bar

Inputs: window_title: the window name of the progress bar
        text: an optional label to add to the progress bar, used to describe the process running
             if text is an empty string, no lable is displayed
***/
void DICOM::setup_progress_bar_dicom(QString window_title, QString text) {
    progress2->reset();
    *remainder2 = 0;
    //this->setDisabled(true);
    progWin2->setWindowTitle(window_title);
    if (!text.isEmpty()) {
        progLabel2->setText(text);
    }
    progWin2->show();
    progWin2->activateWindow();
    progWin2->raise();

}




/***
Function: updateProgress (from martin's interface.cpp 3ddose tools)
-------------------------
Process: Used to update the progress bar
***/
void Interface::updateProgress(double n) {
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
Function: updateProgress (from martin's interface.cpp 3ddose tools)
-------------------------
Process: Used to update the progress bar
***/
void DICOM::updateProgress(double n) {
    // The flooring function rounds down a real number to the nearest integer
    // In this line, we remove the remainder from the total number
    *remainder2 += n - floor(n);

    progress2->setValue(int(progress2->value() + floor(n) + floor(*remainder2))); //Incremeent
    progress2->update(); // We redraw the bar
    QApplication::processEvents();

    // And if our remainder makes a whole number, remove it
    *remainder2 -= floor(*remainder2);
}

void Interface::checkTG43simul() {
    if (tg43_simul_check->isChecked()) {
        tas_list->setDisabled(true);
        select_tas_bttn->setDisabled(true);
    }
    else {
        tas_list->setEnabled(true);
        select_tas_bttn->setEnabled(true);
    }
}

void Interface::checkTG43() {
    if (tg43_simul->isChecked()) {
        tas_selection->setDisabled(true);
        select_tas->setDisabled(true);
    }
    else {
        tas_selection->setEnabled(true);
        select_tas->setEnabled(true);
    }
}


/***
Function: select_tas_file
-----------------------------
Process: Asks user to select a phantom file
         Name of file is added and selected in the QListWidget

***/
void Interface::select_tas_file() {

    //Different actions based on which button pushed
    QPushButton *buttonSender = qobject_cast<QPushButton *>(sender()); // retrieve the button you have clicked
    QString buttonText = buttonSender->text(); // retrive the text from the button clicked

    //"Upload the phantom" is the extra options tab
    //"Upload a phantom" is the using egs_brachy lib tab

    QString tas_file = QFileDialog::getOpenFileName(
                           this,
                           tr("Select the tissue assignment scheme"),
                           egs_brachy_home_path,
                           "(*.txt)");

    if (!tas_file.isEmpty()) {
        QFileInfo tas = QFileInfo(tas_file);

        tas_names.prepend(tas.completeBaseName());
        tas_names_path.prepend(tas.absoluteFilePath());

        if (buttonText == "Upload a TAS") {
            tas_list->insertItem(0, tas.fileName());
            tas_list->setCurrentRow(0);
            tas_selection->insertItem(0, tas.fileName());
        }
        else if (buttonText == "Upload the TAS") {
            tas_selection->insertItem(0, tas.fileName());
            tas_selection->setCurrentRow(0);
            tas_list->insertItem(0, tas.fileName());
        }

    }
    else {
        return;
    }
}


/***
Function: select_phantom_file
-----------------------------
Process: Asks user to select a phantom file
         Name of file is added and selected in the QListWidget

***/
void Interface::select_phantom_file() {

    //Different actions based on which button pushed
    QPushButton *buttonSender = qobject_cast<QPushButton *>(sender()); // retrieve the button you have clicked
    QString buttonText = buttonSender->text(); // retrive the text from the button clicked

    //"Upload the phantom" is the extra options tab
    //"Upload a phantom" is the using egs_brachy lib tab

    QString phant_file = QFileDialog::getOpenFileName(
                             this,
                             tr("Select the phantom"),
                             egs_brachy_home_path,
                             "Phantom (*.geom *.egsphant *.egsphant.gz)");

    if (!phant_file.isEmpty()) {
        QFileInfo phantom = QFileInfo(phant_file);

        phant_names.prepend(phantom.completeBaseName());
        phant_names_path.prepend(phantom.absoluteFilePath());

        if (buttonText == "Upload a phantom") {
            phant_selection->insertItem(0, phantom.fileName());
            phant_selection->setCurrentRow(0);
            phant_selection_inputfiles->insertItem(0, phantom.fileName());
        }
        else if (buttonText == "Upload the phantom") {
            phant_selection_inputfiles->insertItem(0, phantom.fileName());
            phant_selection_inputfiles->setCurrentRow(0);
            phant_selection->insertItem(0, phantom.fileName());
        }

    }
    else {
        return;
    }
}




/***
Function: select_transform_file
-----------------------------
Process: Asks user to select a seed transformation file
         Name of file is added and selected in the QListWidget

***/
void Interface::select_transform_file() {
    //Different actions based on which button pushed
    QPushButton *buttonSender = qobject_cast<QPushButton *>(sender()); // retrieve the button you have clicked
    QString buttonText = buttonSender->text(); // retrive the text from the button clicked

    //"Upload the seed transformation file" is the extra options tab
    //"Upload a seed transformation file" is the using egs_brachy lib tab
    QString transform_file = QFileDialog::getOpenFileName(
                                 this,
                                 tr("Select the seed transformation file"),
                                 egs_brachy_home_path);

    if (!transform_file.isEmpty()) {
        QFileInfo transform = QFileInfo(transform_file);

        seed_names.prepend(transform.fileName());
        seed_names_path.prepend(transform.absoluteFilePath());

        if (buttonText == "Upload a seed transformation file") {
            seed_selection->insertItem(0, transform.fileName());
            seed_selection->setCurrentRow(0);
            seed_selection_inputfiles->insertItem(0, transform.fileName());
        }
        else if (buttonText == "Upload the seed transformation file") {
            seed_selection_inputfiles->insertItem(0, transform.fileName());
            seed_selection_inputfiles->setCurrentRow(0);
            seed_selection->insertItem(0, transform.fileName());
        }


    }
    else {
        return;
    }

}

/***
Function: select_transport_file
-----------------------------
Process: Asks user to select a transport file
         Name of file is added and selected in the QListWidget

***/
void Interface::select_transport_file() {

    QString transport_file = QFileDialog::getOpenFileName(
                                 this,
                                 tr("Select the transport file"),
                                 egs_brachy_home_path);

    if (!transport_file.isEmpty()) {
        QFileInfo transform = QFileInfo(transport_file);

        transport_names.prepend(transform.fileName());
        transport_names_path.prepend(transform.absoluteFilePath());


        transport_selection->insertItem(0, transform.fileName());
        transport_selection->setCurrentRow(0);

    }
    else {
        return;
    }

}

/***
Function: enable_midTab_trim
----------------------------
Process: Allows the user to trim/inscribe geom in middle tab
***/
void Interface::enable_midTab_trim(int row) {
    if (row != -1) {
        QVector <double> emptyBounds(3, 0);
        QString phant_file = phant_names_path[row];

        if (phant_file.endsWith(".egsphant.gz") || phant_file.endsWith(".egsphant")) {

            EGSPhant *selectedPhant= new EGSPhant;

            if (phant_file.endsWith(".egsphant.gz")) { //gunzip the file
                QProcess *qzip = new QProcess();
                qzip->setProcessChannelMode(QProcess::MergedChannels);
                qzip->setReadChannel(QProcess::StandardOutput);

                //Actual line of exectuion
                QString command = "gunzip " + phant_file;
                qzip->execute(command);

                phant_file.chop(3);
                selectedPhant->loadEGSfirstlines(phant_file);

                //zip back up
                command = "gzip -f " + phant_file;  //-f used if exisitng .gz file, -f forces the
                qzip->execute(command);
            }
            else {
                selectedPhant->loadEGSfirstlines(phant_file);
            }

            trim_phantom_inputfiles->setEnabled(true);
            trim_phantom_inputfiles->setToolTip(tr("This button allows the user to change the egsphant boundaries, inscribe\n") +
                                                tr("geometries within the egsphant or inscribe the phant within a geometry.\n"));
            if (selectedPhant->x.size() < 2 || selectedPhant->y.size() <2 || selectedPhant->z.size() <2) {
                std::cout<<"The selected phanton cannot be trimmed as one or more boundaires is less than two voxels \n";
                trimEGSMid = new trim(this, emptyBounds, emptyBounds, emptyBounds);
                trimEGSMid->egs_path = egs_brachy_home_path;
            }
            else {
                trimEGSMid = new trim(this, selectedPhant->x, selectedPhant->y, selectedPhant->z);
                trimEGSMid->egs_path = egs_brachy_home_path;
            }
        }
        else {   //the selected phant isn't a .egsphant -can't trim
            trim_phantom_inputfiles->setEnabled(true);
            trim_phantom_inputfiles->setToolTip(tr("This button allows the user to inscribe\n") +
                                                tr("geometries within the egsphant or inscribe the phant within a geometry.\n"));
            trimEGSMid = new trim(this, emptyBounds, emptyBounds, emptyBounds);
            trimEGSMid->egs_path = egs_brachy_home_path;
        }
    }
    else {
        trim_phantom_inputfiles->setDisabled(true);
        trim_phantom_inputfiles->setToolTip(tr("This button is enabled when a phant file is selected from above."));
    }
}

void Interface::showTrim_midtab() {
    trimEGSMid->save_options_on_startupMid("");
    trimEGSMid->window->show();

}

/***
Function: run_brachy_phantom
----------------------------
Process: Verifies all selections have been made and creates the egsinp files
***/
void Interface::run_brachy_phantom() {
    this->setDisabled(true);

    //Add error if phant and/or seed and/or transp param aren't selected
    bool missing_seed = false;
    bool missing_phant = false;
    bool missing_transport = false;
    int count_misisng = 0;

    if (seed_selection->currentRow() == -1) {
        missing_seed = true;
        count_misisng ++;
    }

    if (phant_selection->currentRow() == -1) {
        missing_phant = true;
        count_misisng ++;
    }

    if (transport_selection->currentRow() == -1) {
        missing_transport = true;
        count_misisng ++;
    }

    //Check the ncase value
    double number_hist = numb_histories->text().toDouble();
    double number_batch = numb_batch->text().toDouble();
    double number_chunk = numb_chunk->text().toDouble();

    //Output the error message if count_missing > 0
    if (count_misisng > 0) {
        //Getting the names of the widgets that aren't selected
        QString message = "Select ";
        if (count_misisng ==1) {
            if (missing_seed) {
                message+="a seed transformation file.";
            }
            if (missing_phant) {
                message+="a seed transformation file.";
            }
            if (missing_transport) {
                message+="a transport parameter file.";
            }
        }
        if (count_misisng ==2) {
            if (missing_seed && missing_phant) {
                message+="a phantom and seed transformation file.";
            }
            if (missing_seed && missing_transport) {
                message+="a seed transformation and transport parameter file.";
            }
            if (missing_transport && missing_phant) {
                message+="a phantom and transport parameter file.";
            }
        }
        if (count_misisng ==3) {
            message+="a phantom, seed transformation and transport parameter file.";
        }

        QMessageBox msgBox;
        msgBox.setText(message);
        msgBox.setWindowTitle(tr("Error"));
        msgBox.exec();


    }
    else if ((number_hist < 1) && (!numb_histories->text().isEmpty())) { //ERROR, must be greater than one, user can try again
        QMessageBox msgBox;
        msgBox.setText(tr("The number of histories must be a positive integer."));
        msgBox.setWindowTitle(tr("Error"));
        msgBox.exec();


    }
    else if ((number_batch < 1) && (!numb_batch->text().isEmpty())) { //ERROR, must be greater than one, user can try again
        QMessageBox msgBox;
        msgBox.setText(tr("The number of batches must be a positive integer."));
        msgBox.setWindowTitle(tr("Error"));
        msgBox.exec();


    }
    else if ((number_chunk < 1) && (!numb_chunk->text().isEmpty())) { //ERROR, must be greater than one, user can try again
        QMessageBox msgBox;
        msgBox.setText(tr("The number of chunks must be a positive integer."));
        msgBox.setWindowTitle(tr("Error"));
        msgBox.exec();


    }
    else {
        //All parameters have been set
        this->setDisabled(true);


        //Get the source models
        bool continue_function = get_seed_from_user();

        if (continue_function == false) { //user has pressed cancel in seed selection
            return;
        }


        //score energy deposition
        bool checked_energy_depos = false;
        if (yes->isChecked()) {
            checked_energy_depos = true;
        }

        //run mode
        QString run_mode;
        if (normal->isChecked()) {
            run_mode = "normal";
        }
        else if (superposition->isChecked()) {
            run_mode = "superposition";
        }
        else if (volume_correction->isChecked()) {
            run_mode = "volume correction only";
        }


        QString seed_file = seed_names_path[seed_selection->currentRow()];
        QString phant_file = phant_names_path[phant_selection->currentRow()];
        QString transp_file = transport_names_path[transport_selection->currentRow()];
        egs_input_phantom->egsinp_path = select_location_phantom->egsinp_path->text();  //setting the egsinp file path
        egs_input_phantom->seed_file = seed_file;

        //Trim the egsphant if user chose to
        if (trimEGSMid->contourBox->isChecked()) {
            phant_file = trimExisitngEGS_Mid(phant_file);
        }


        //egsinp file name
        if (select_location_phantom->changed_location == true) { //user has changed the egsinp file location
            egs_input_phantom->egsinp_path = select_location_phantom->egsinp_path->text();
        }


        //Getting the egsinp file name (without the filename extension)
        int n = egs_input_phantom->egsinp_path.count("/");
        QString egsinp_folder_name = egs_input_phantom->egsinp_path.section("/", n, n);
        int period = egsinp_folder_name.lastIndexOf(".");
        egs_input_phantom->egsinp_name = egsinp_folder_name.left(period);

        std::cout<<"Warning: the dose scaling factor (DSF) and source weights are missing from the egsinp file.\n";
        egs_input_phantom->boxFilePath = trimEGSMid->inscribedPhantPath;
        egs_input_phantom->applicatorFilePath = trimEGSMid->applicatorPath;
        //Creating and outputting the egsinp -different file based on if phant or egsphant used
        //----------------------------------
        egs_input_phantom->create_egsinp_phantom(element, seed, phant_file,
                transp_file, run_mode, checked_energy_depos, selected_muen_file, number_hist,
                number_batch, number_chunk, select_location_phantom->egsinp_header, selected_material_file);

        //Launch egs_brachy
        //----------------------------------
        this->setDisabled(true);
        show_launch_egs_phantom();

    }

    this->setEnabled(true);
}


/***
Function: load_muen
-------------------
Asks the user to select the muen file to upload
***/
void Interface::load_muen() {
    this->setDisabled(true);

    QString muen_file = QFileDialog::getOpenFileName(
                            this,
                            tr("Select muen file to open"),
                            egs_brachy_home_path,
                            "muen (*.muendat)");

    if (!muen_file.isEmpty()) {
        selected_muen_file = muen_file;
    }

    this->setEnabled(true);
}

/***
Function: load_material data file
-------------------
Asks the user to select the muen file to upload
***/
void Interface::load_material() {
    this->setDisabled(true);

    QString material_file = QFileDialog::getOpenFileName(
                                this,
                                tr("Select material to open"),
                                egs_brachy_home_path,
                                "dat (*.dat)");

    if (!material_file.isEmpty()) {
        selected_material_file = material_file;
    }

    this->setEnabled(true);
}

/***
Function: change_phantom_file_location
--------------------------------------
Process: Allows the user to update the location where the egsinp file is saved

***/
void Interface::change_phantom_file_location() {
    this->setDisabled(true);
    select_location_phantom->save_info_startup();

    select_location_phantom->window->show();
    QApplication::processEvents();
    select_location_phantom->setEnabled(TRUE);

}






/*********************************************************************************
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Extra Options Tab~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*******************************************************************************/

//--------------------------------------------------
//--------------------------------------------------
//      From DICOM data
//--------------------------------------------------
//--------------------------------------------------


/***
Function: loadDICOMfile
----------------
Process: This function asks the user  for a DICOM files and parses the dicom data
         to extract all the relevant/necessary information
***/
void Interface::loadDICOMfile() {

    egs_input_file = new egsinp;

    //Initializing default files
    egs_input_file->muen_file_substring = muen_file_substring;
    egs_input_file->low_energy_transport_file = low_energy_transport_file;
    egs_input_file->high_energy_transport_file = high_energy_transport_file;
    egs_input_file->material_data = material_data_file;
    egs_input_file->spectra_files = spectrum_files;

    egs_input_file->egs_home = egs_brachy_home_path;
    phantBox_inputfiles->setDisabled(true);
    calibrationButton->setDisabled(true);
    create_input_files_button->setDisabled(true);
    seedBox_inputfiles->setDisabled(true);
    seedBox_inputfiles->setToolTip(tr("If a DICOM CT files are loaded and a DICOM Plan file\n") +
                                   tr("isn't, the seed transformation pane will become enabled.\n")+
                                   tr("The selection is used to create the \n") +
                                   tr("egsinp file."));
    phantBox_inputfiles->setToolTip(tr("If a DICOM Plan file is loaded and DICOM CT \n") +
                                    tr("data isn't, the phantom pane will become enabled.\n")+
                                    tr("The selection is used to create the \n") +
                                    tr("egsinp file."));
    //creating_input_rt->set_source_folder(source_folder_path);
    tas_grid->setDisabled(true); //used in case input files have been created and then user
    //clicks to load dicom files (previewer can no longer be accessed)

    QString dir_path= QFileDialog::getExistingDirectory(
                          this,
                          tr("Select a DICOM directory to open"),
                          "/data/data060/sjarvis/Dicom_data/");
    // QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);

    QVector <QString> tempS2;
    QDir dir(dir_path);
    QFileInfoList fileNamesRT = dir.entryInfoList();
    QDirIterator directories(dir_path, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    //Reads all the files in sub-directories of the dir_path
    while (directories.hasNext()) { //one more entry in the directory

        QFile f(directories.next());    //advances iterator to next entry
        QDir dir1(f.fileName());
        QFileInfoList fileNam = dir1.entryInfoList();   //all files and directories in the sub-directory

        for (int i=0; i < fileNam.size(); i ++) {
            QFileInfo fileInf = fileNam.at(i);

            if (fileInf.isFile()) { //Adds the file to the list of file names
                fileNamesRT.push_back(fileInf);
            }
        }
    }


    //reading each DICOM file in the directory
    if (!fileNamesRT.isEmpty() && !dir_path.isEmpty()) {
        for (int i=0; i < fileNamesRT.size(); i ++) {
            QFileInfo fileInfo = fileNamesRT.at(i);
            if (fileInfo.isFile()) {
                tempS2.append(fileInfo.absoluteFilePath());
            }
        }

        get_data = new DICOM;
        get_data ->extract(tempS2);

        if (get_data->loadedct) {
            // Assume first slice matches the rest and set x, y, and z boundaries
            phant.nx = get_data->xPix[0];
            phant.ny = get_data->yPix[0];
            phant.nz = get_data->numZ;
            phant.x.fill(0,phant.nx+1);
            phant.y.fill(0,phant.ny+1);
            phant.z.fill(0,phant.nz+1);

            {
                QVector <char> mz(phant.nz, 0);
                QVector <QVector <char> > my(phant.ny, mz);
                QVector <QVector <QVector <char> > > mx(phant.nx, my);
                phant.m = mx;
                QVector <double> dz(phant.nz, 0);
                QVector <QVector <double> > dy(phant.ny, dz);
                QVector <QVector <QVector <double> > > dx(phant.nx, dy);
                phant.d = dx;
                QVector <int> sz(phant.nz, 0);
                QVector <QVector <int> > sy(phant.ny, sz);
                QVector <QVector <QVector <int> > > sx(phant.nx, sy);
                phant.contour = sx;
            }

            // Define xy bound values, still assuming first slice matches the rest
            for (int i = 0; i <= phant.nx; i++) {
                phant.x[i] = (get_data->imagePos[0][0]+(i-0.5)*get_data->xySpacing[0][0])/10.0;
            }

            for (int i = 0; i <= phant.ny; i++) {
                phant.y[i] = (get_data->imagePos[0][1]+(i-0.5)*get_data->xySpacing[0][1])/10.0;
            }


            // Define z bound values
            double prevZ, nextZ;
            nextZ = get_data->imagePos[0][2]-get_data->zSpacing[0]/2.0;
            for (int i = 0; i < phant.nz; i++) {
                prevZ = nextZ/2.0 + (get_data->imagePos[i][2]-get_data->zSpacing[i]/2.0)/2.0;
                nextZ = get_data->imagePos[i][2]+get_data->zSpacing[i]/2.0;
                phant.z[i] = prevZ/10.0;
            }
            phant.z.last() = nextZ/10.0;
        }

        //enable assignment scheme selection
        if (get_data->loadedct && get_data->loadedstruct) { //Create egsphant using TAS
            phantBox_inputfiles->setDisabled(true);
            phantBox_inputfiles->setToolTip(tr("Phantom selection disabled as CT file(s)\n") +
                                            tr("have been loaded."));
            tas_grid->setEnabled(true);
            tas_grid->setToolTip(tr("Selecting the tissue assignment scheme determines which\n")+
                                 tr("organs and tissues are assigned in the CT data."));
            calibrationButton->setEnabled(true);
            calibrationButton-> setToolTip(tr("This button allows the user to load a\n") +
                                           tr("CT calibration file (HU to density). \n") +
                                           tr("If no file is uploaded, a default calibration is used."));
            create_input_files_button->setEnabled(true);
        }
        else if (get_data->loadedct && !get_data->loadedstruct) { //Create phant using TG-43
            phantBox_inputfiles->setDisabled(true);
            phantBox_inputfiles->setToolTip(tr("Phantom selection disabled as CT file(s)\n") +
                                            tr("have been loaded"));
            tas_grid->setDisabled(true);
            tas_grid->setToolTip(tr("Selecting the tissue assignment scheme disabled\n")+
                                 tr("as DICOM structure file was not loaded."));
            calibrationButton->setEnabled(true);
            calibrationButton-> setToolTip(tr("This button allows the user to load a\n") +
                                           tr("CT calibration file (HU to density). \n") +
                                           tr("If no file is uploaded, a default calibration is used."));
            create_input_files_button->setEnabled(true);
        }

        if (get_data->loadedplan) {
            seedBox_inputfiles->setDisabled(true);
            seedBox_inputfiles->setToolTip(tr("The seed transformation selection is used for the egsinp file.\n")+
                                           tr(" "));
            create_input_files_button->setEnabled(true);
        }
        else if (!get_data->loadedplan) {
            seedBox_inputfiles->setEnabled(true);
            seedBox_inputfiles->setToolTip(tr("The seed transformation selection is used for the egsinp file.\n")+
                                           tr(" "));
            create_input_files_button->setEnabled(true);
        }

        if (get_data->loadedstruct && !get_data->loadedplan && !get_data->loadedct) {
            create_input_files_button->setDisabled(true);
            seedBox_inputfiles->setDisabled(true);
            QMessageBox msgBox;
            msgBox.setText(tr("Unable to create egs_brachy input files with just a DICOM Struct file."));
            msgBox.setInformativeText(tr("To generate egs_brachy input files, please select a DICOM directory containing more than just a DICOM Struct file"));
            msgBox.setWindowTitle(tr("egs_brachy GUI"));
            msgBox.exec();

        }
    }

    this->setEnabled(true);

}






/***
Function: setup_input_files
----------------------------------
Process:Prompts user to load a  DICOM folder
        Extracts all necessary information from the DICOM files

***/
void Interface::setup_input_files() {
    //this->setDisabled(true);
    createEgsinp = true;
    extraOptionsFlag = true;
    //User needs to select a phantom file
    if (!get_data->loadedct && phant_selection_inputfiles->currentRow() == -1 && phant_selection_inputfiles->isEnabled()) {
        QMessageBox msgBox;
        msgBox.setText(tr("Would you like to create an egsinp file?"));
        msgBox.setInformativeText(tr("Please select a phantom from the list or upload a DICOM directory containing CT files"));
        QAbstractButton *noEgsinpButton = msgBox.addButton(tr("Continue without creating an egsinp file"), QMessageBox::AcceptRole);
        //QAbstractButton *selectPhantButton = msgBox.addButton(tr("Cancel and select a phantom"), QMessageBox::RejectRole); // unused
        msgBox.setDefaultButton(QMessageBox::Save);
        msgBox.exec();

        if (msgBox.clickedButton() == noEgsinpButton) {
            createEgsinp = false;
        }
        else {
            this->setEnabled(true);
            return;
        }

    }
    else if (!get_data->loadedplan && seed_selection_inputfiles->currentRow() == -1 && seed_selection_inputfiles->isEnabled()) {
        QMessageBox msgBox;
        msgBox.setText(tr("Would you like to create an egsinp file?"));
        msgBox.setInformativeText(tr("Please select a seed transformation file from the list or upload a DICOM directory containing a Plan file"));
        QAbstractButton *noEgsinpButton = msgBox.addButton(tr("Continue without creating an egsinp file"), QMessageBox::AcceptRole);
        //QAbstractButton *selectPhantButton = msgBox.addButton(tr("Cancel and select a transformation file"), QMessageBox::RejectRole); // unused
        msgBox.setDefaultButton(QMessageBox::Save);
        msgBox.exec();

        if (msgBox.clickedButton() == noEgsinpButton) {
            createEgsinp = false;
        }
        else {
            this->setEnabled(true);
            return;
        }
    }
    else if (!get_data->loadedct && !phant_selection_inputfiles->isEnabled() && !seed_selection_inputfiles->isEnabled()) {
        createEgsinp = false;
    }

    QString file_path_transformation;
    QString file_path_egsinp;
    QString file_path_egsphant;

    //Prompt user to enter egsphant save file location
    if (get_data->loadedct) {
        file_path_egsphant = egs_brachy_home_path + "/egsphant.egsphant";
    }

    if (get_data->loadedplan) {
        file_path_transformation = egs_brachy_home_path + "/transformation_file";
    }

    if (createEgsinp) {
        file_path_egsinp = egs_brachy_home_path + "/egsinp.egsinp";
    }



    select_location_additional_function = new File_selector(this, egs_brachy_home_path,
            file_path_egsinp, file_path_egsphant, file_path_transformation);


    select_location_additional_function->window->show();
    QApplication::processEvents();
    select_location_additional_function->setEnabled(TRUE);

    connect(select_location_additional_function, SIGNAL(cancel_pressed()),
            this, SLOT(stop_process()));

    connect(select_location_additional_function, SIGNAL(closed()),
            this, SLOT(create_input_files()));


}



/***
Function: create_input_files
----------------------------
Process: Based on the dicom files loaded, the corresponding egs_brachy input files are created:
            -CT: egsphant
            -Plan: egsinp, seed transformation file
***/
void Interface::create_input_files() {
    //Get the seed -needed for the seed transformation file (air kerma) and egsinp
    if (get_data->loadedplan || createEgsinp) {
        bool continue_function = get_seed_from_user();

        if (continue_function == false) { //user has pressed cancel in seed selection
            return;
        }
    }

    //---------------------------------------------------
    //  GENERATE THE PHANTOM
    //---------------------------------------------------
    //If loaded CT files and not struct, or TG43 is selected
    if ((get_data->loadedct && !get_data->loadedstruct) || (get_data->loadedct && tg43_simul->isChecked())) {
        egs_input_file->egsphant_location = select_location_additional_function->egsphant_path->text();
        AT_tg43Flag = true;  //it's a TG-43 simul
        AT_TAS_file = working_path +"/Tissue_Assignment_Schemes/tg43.txt";

        structure.clear();
        tissue.clear();
        density_max.clear();
        structUnique.clear();

        //Read the TG43 TAS file
        if (!read_TAS_file(AT_TAS_file, false)) {
            structure << "Water";
            tissue << "WATER_0.998";
            density_max << 100;
            std::cout<<"Using default TG-43 media " <<tissue.last().toStdString() <<"\n";
        }

        structUnique.append(false);
        setup_egsphant();

    }
    else if (get_data->loadedct && get_data->loadedstruct) { //If loaded ct files and struct, create egsphant 'normally'
        egs_input_file->egsphant_location = select_location_additional_function->egsphant_path->text();

        if (tas_list->currentRow() != -1) {
            AT_TAS_file = tas_names_path[tas_selection->currentRow()];
        }
        else {
            AT_tg43Flag = true;  //it's a TG-43 simul
            AT_TAS_file = working_path +"/Tissue_Assignment_Schemes/tg43.txt";
        }

        structure.clear();
        tissue.clear();
        density_max.clear();
        structUnique.clear();

        if (AT_tg43Flag == true) {

            if (!read_TAS_file(AT_TAS_file, false)) {
                structure << "Water";
                tissue << "WATER_0.998";
                density_max << 100;
                std::cout<<"Using default TG-43 media " <<tissue.last().toStdString() <<"\n";
            }

            structUnique.append(false);
            setup_egsphant();

        }
        else {

            while (!read_TAS_file(AT_TAS_file, get_data->externalStruct)) {
                QMessageBox msgBox;
                msgBox.setText(tr("Please select a valid tissue assignment scheme (.txt) file."));
                msgBox.setWindowTitle(tr("egs_brachy GUI"));
                msgBox.exec();

                AT_TAS_file = QFileDialog::getOpenFileName(
                                  this,
                                  tr("Select a valid Tissue Assignment Scheme"),
                                  working_path + "/Tissue_Assignment_Schemes/",
                                  "Text (*.txt)");

                if (AT_TAS_file.isEmpty()) { //user clicked cancel
                    return;
                }
            }

            //Ask user to verify the tissue assignment scheme
            QVector<QString> unique_struct;
            for (int i=0; i<structure.size(); i++) {
                if (!unique_struct.contains(structure[i])) {
                    unique_struct.append(structure[i]);
                }
            }

            tissue_check = new Tissue_check(this, get_data->structName, unique_struct, get_data->structType);

            //tissue_check->setDisabled(TRUE);
            tissue_check->window->show();
            QApplication::processEvents();
            tissue_check->setEnabled(TRUE);

            connect(tissue_check, SIGNAL(closed()), this, SLOT(set_tissue_selection()));
            connect(tissue_check, SIGNAL(exit()), this, SLOT(stop_process()));

            tissue_check->setDisabled(TRUE);
            QApplication::processEvents();

        }

    }
    else {
        continue_AT_egsinp();
    }
}

void Interface::continue_AT_egsinp() {

    //---------------------------------------------------
    //  GENERATE THE SEED TRANSFORM fILE
    //---------------------------------------------------
    //If loaded plan file, create seed transformation
    if (get_data->loadedplan) {

        egs_input_file->seed_file = select_location_additional_function->transformation_path->text();
        DSF = get_data->DSF_intermediate/(air_kerma_per_history*100);
        egs_input_file->create_transformation_file(get_data->all_seed_pos, get_data->air_kerma);

    }

    //---------------------------------------------------
    //  GENERATE THE EGSINP
    //---------------------------------------------------
    if (createEgsinp) {
        egs_input_file->egsinp_path = select_location_additional_function->egsinp_path->text();

        if (egs_input_file->seed_file.isEmpty()) {
            egs_input_file->seed_file = seed_names_path[seed_selection_inputfiles->currentRow()];
        }

        if (egs_input_file->egsphant_location.isEmpty()) {
            egs_input_file->egsphant_location = phant_names_path[phant_selection_inputfiles->currentRow()];
        }


        //Save the source weights and DSF
        //------------------------------
        QVector<double> source_weights;
        if (get_data->loadedplan) {
            if (get_data->treatment_type == "HDR") {
                double max_weight = 0;
                for (int i=0; i<get_data->all_seed_time.size(); i++) {
                    for (int j=0; j<get_data->all_seed_time[i].size(); j++) {
                        source_weights.append(round((get_data->all_seed_time[i][j]/get_data->total_time)*10000.0)/10000.0);
                        if (source_weights.last() > max_weight) {
                            max_weight = source_weights.last();
                        }
                    }
                }
                DSF = DSF*max_weight;
            }
        }
        else {
            std::cout<<"Warning: no DICOM plan file has been loaded. The source weights and dose scaling factor will NOT be included in the egsinp file. \n";
            DSF = 0;
        }

        //Creating and outputting the egsinp
        //----------------------------------
        QString empty;

        egs_input_file->create_egsinp(element, seed, DSF, get_data->treatment_type,
                                      source_weights, phant.media, AT_tg43Flag,
                                      false, egs_input_file->muen_file_substring, 1e8,  1, 10, select_location_additional_function->egsinp_header, empty, empty);
    }

    delete egs_input_file;

}


/***
Function::create_egsinp
----------------------
Process: Creates the egsinp files
***/
void Interface::ATcreate_egsphant() {
    this->setDisabled(true);

    if (structUnique.contains(true)) {
        structPrio = get_prio->structPrio;
    }
    // ---------------------------------------------------------- //
    // GENERATE EGSPHANT AND HU CONVERSION MAPS/THRESHOLDS        //
    // ---------------------------------------------------------- //
    std::clock_t start;
    double duration;
    start = std::clock();


    // ---------------------------------------------------------- //
    // SETUP THE STRUCTURES AND MEDIA                             //
    // ---------------------------------------------------------- //
    QMap <QString,unsigned char> mediaMap;
    phant.media.clear();

    if (AT_tg43Flag) {

        // Track the number of media, and create a lookup for media to ASCII character representation
        int medNum = 0;
        phant.media.append(tissue[0]);
        mediaMap.insert(tissue[0], 49 + medNum + (medNum>8?7:0) + (medNum>34?6:0));


        // ---------------------------------------------------------- //
        // CONVERTING HU TO APPROPRIATE DENSITY AND MEDIUM            //
        // ---------------------------------------------------------- //
        setup_progress_bar("Generating the phantom", "");

        double increment =  1000000000/(phant.nx*phant.nz +3);

        updateProgress(increment);
        media = phant.media;

        updateProgress(2*increment);

        // Arrays that hold the struct numbers and center voxel values to be used
        QList<QPoint> zIndex, yIndex, zExtIndex, yExtIndex;
        QList<QPoint>::iterator p;
        // double zMid, yMid, xMid; // unused
        int tempHU = 0, n = 0, q = 0; // , inStruct = 0, prio = 0; // unused

        // Convert HU to density and media without masks
        for (int k = 0; k < phant.nz; k++) { // Z //
            for (int j = 0; j < phant.ny; j++) { // Y //
                for (int i = 0; i < phant.nx; i++) { // X //
                    tempHU = get_data->HU[k][j][i];

                    // Linear search because I don't think these arrays every get big
                    // get the right density
                    for (n = 0; n < HUMap.size()-1; n++)
                        if (HUMap[n] <= tempHU && tempHU < HUMap[n+1]) {
                            break;
                        }
                    if (tempHU < HUMap[0]) {
                        n = 0;
                    }
                    if (tempHU > HUMap[HUMap.size()-1]) {
                        n = HUMap.size() -2;
                    }

                    double temp = interp(tempHU,HUMap[n],HUMap[n+1],denMap[n],denMap[n+1]);
                    temp = temp<=0?0.000001:temp; // Set min density to 0.000001

                    if (temp > phant.maxDensity) { // Track max density for images
                        phant.maxDensity = temp;
                    }

                    //STR//-------------------------------------------------------
                    //if in the selected contour, replace with low threshold value
                    if (phant.d[i][j][k] == 0) {
                        phant.d[i][j][k] = temp;    //assign density
                    }
                    else {
                        temp = phant.d[i][j][k];
                    }

                    if (setup_MAR_Flag && q == indexMARContour && phant.d[i][j][k] < low_threshold) {
                        phant.d[i][j][k] = replacement;
                    }
                    //-------------------------------------------

                    phant.m[i][j][k] = 49;

                }
            }
            updateProgress(increment);
        }



        duration = (std::clock()-start)/(double)CLOCKS_PER_SEC;
        std::cout << "Successfully generated egsphant (dimensions x: [" << phant.x[0] << "," << phant.x[phant.nx] << "], y:["
                  << phant.y[0] << "," << phant.y[phant.ny] << "], z:["
                  << phant.z[0] << "," << phant.z[phant.nz] << "]).  Time elapsed is " << duration << " s.\n";


        progress->setValue(1000000000);
        progWin->hide();
        progWin->close();

    }
    else {

        //Assign enternal contours a default priority
        for (int i = 0; i < get_data->structName.size(); i++) {
            if (get_data->structType[i] ==  "EXTERNAL") {
                structUnique[i] = true;
                structPrio[i] = 1;
                external[i] = true;
            }
        }

        // Track the number of media, and create a lookup for media to ASCII character representation
        int medNum = 0;

        for (int i = 0; i < medThreshold.size(); i++) {
            if (!mediaMap.contains(medThreshold[i])) {
                phant.media.append(medThreshold[i]);
                mediaMap.insert(medThreshold[i], 49 + medNum + (medNum>8?7:0) + (medNum>34?6:0));
                medNum++; // 49 is the ASCII value of '1', 7 jumps from ';' to 'A', 6 jumps from '[' to 'a'
            }
        }

        for (int i = 0; i < get_data->structName.size(); i++) {
            for (int j = 0; j < medThresholds[i].size(); j++)
                if (!mediaMap.contains(medThresholds[i][j])) {
                    phant.media.append(medThresholds[i][j]);
                    mediaMap.insert(medThresholds[i][j], 49 + medNum + (medNum>8?7:0) + (medNum>34?6:0));
                    medNum++; // 49 is the ASCII value of '1', 7 jumps from ';' to 'A', 6 jumps from '[' to 'a'
                }
        }


        // ---------------------------------------------------------- //
        // CONVERTING HU TO APPROPRIATE DENSITY AND MEDIUM            //
        // ---------------------------------------------------------- //
        setup_progress_bar("Generating the phantom", "");

        double increment =  1000000000/(phant.nx*phant.nz +3);

        updateProgress(increment);
        media = phant.media;


        // Get bounding rectangles over each struct
        QVector <QVector <QRectF> > structRect;
        for (int i = 0; i < get_data->structPos.size(); i++) {
            structRect.resize(i+1);
            for (int j = 0; j < get_data->structPos[i].size(); j++) {
                structRect[i].resize(j+1);
                structRect[i][j] = get_data->structPos[i][j].boundingRect();
            }
        }


        updateProgress(increment);


        updateProgress(increment);
        // Arrays that hold the struct numbers and center voxel values to be used
        QList<QPoint> zIndex, yIndex, zExtIndex, yExtIndex;
        QList<QPoint>::iterator p;
        double zMid, yMid, xMid;
        int tempHU = 0, n = 0, q = 0, inStruct = 0, prio = 0;

        // Convert HU to density and media without masks
        for (int k = 0; k < phant.nz; k++) { // Z //
            if (get_data->structZ.size() > 0) {
                zIndex.clear(); // Reset lookup
                zMid = (phant.z[k]+phant.z[k+1])/2.0;
                for (int l = 0; l < get_data->structZ.size(); l++)
                    if (structUnique[l])
                        for (int m = 0; m < get_data->structZ[l].size(); m++) {
                            // If slice j of struct i on the same plane as slice k of the phantom
                            if (abs(get_data->structZ[l][m] - zMid) < (phant.z[k+1]-phant.z[k])/2.0) {
                                zIndex << QPoint(l,m);    // Add it to lookup
                            }
                        }
            }

            for (int j = 0; j < phant.ny; j++) { // Y //
                if (zIndex.size() > 0) {
                    yIndex.clear(); // Reset lookup
                    yMid = (phant.y[j]+phant.y[j+1])/2.0;
                    for (p = zIndex.begin(); p != zIndex.end(); p++) {
                        // If column p->y() of struct p->x() on the same column as slice k,j of the phantom
                        if (structRect[p->x()][p->y()].top() <= yMid && yMid <= structRect[p->x()][p->y()].bottom()) {
                            yIndex << *p;
                        }
                    }
                }

                for (int i = 0; i < phant.nx; i++) { // X //
                    tempHU = get_data->HU[k][j][i];
                    xMid = (phant.x[i]+phant.x[i+1])/2.0;

                    // Linear search because I don't think these arrays every get big
                    // get the right density
                    for (n = 0; n < HUMap.size()-1; n++)
                        if (HUMap[n] <= tempHU && tempHU < HUMap[n+1]) {
                            break;
                        }
                    if (tempHU < HUMap[0]) {
                        n = 0;
                    }
                    if (tempHU > HUMap[HUMap.size()-1]) {
                        n = HUMap.size() -2;
                    }


                    double temp = interp(tempHU,HUMap[n],HUMap[n+1],denMap[n],denMap[n+1]);
                    temp = temp<=0?0.000001:temp; // Set min density to 0.000001

                    if (temp > phant.maxDensity) { // Track max density for images
                        phant.maxDensity = temp;
                    }

                    //if in the selected contour, replace with low threshold value
                    if (phant.d[i][j][k] == 0) {
                        phant.d[i][j][k] = temp; //assign density
                    }
                    else {
                        temp = phant.d[i][j][k];
                    }

                    // get the right media
                    if (yIndex.size() > 0) {
                        inStruct = 0;
                        prio = 0;
                        for (p = yIndex.begin(); p != yIndex.end(); p++) { // Check through each
                            // If row p->y() of struct p->x() on the same row as slice k,j,i of the phantom
                            if (structRect[p->x()][p->y()].left() <= xMid && xMid <= structRect[p->x()][p->y()].right())
                                if (get_data->structPos[p->x()][p->y()].containsPoint(QPointF(xMid,yMid), Qt::OddEvenFill)) {
                                    if (structPrio[p->x()] > prio) {
                                        inStruct = p->x()+1; // Inflate index for the next check
                                        prio = structPrio[p->x()]; // Set priority at this struct's prio
                                    }
                                }
                        }
                    }

                    if (inStruct) { // Deflate index again
                        inStruct--;

                        q = get_data->structLookup[get_data->structReference[inStruct]]; // get the structName index which matches denThresholds

                        //Flag if in the selected contour for STR
                        if (setup_MAR_Flag && q == indexMARContour && phant.d[i][j][k] < low_threshold) {
                            phant.d[i][j][k] = replacement;
                        }

                        //Find the media of the voxel
                        for (n = 0; n < denThresholds[q].size()-1; n++)
                            if (temp < denThresholds[q][n]) {
                                break;
                            }

                        phant.m[i][j][k] = mediaMap[medThresholds[q][n]];
                        phant.contour[i][j][k] = q;

                    }
                    else {
                        for (n = 0; n < denThreshold.size()-1; n++)
                            if (temp < denThreshold[n]) {
                                break;
                            }
                        phant.m[i][j][k] = 49 + n + (n>8?7:0) + (n>34?6:0);
                    }
                }
                updateProgress(increment);
            }
        }


        duration = (std::clock()-start)/(double)CLOCKS_PER_SEC;
        std::cout << "Successfully generated egsphant (dimensions x: [" << phant.x[0] << "," << phant.x[phant.nx] << "], y:["
                  << phant.y[0] << "," << phant.y[phant.ny] << "], z:["
                  << phant.z[0] << "," << phant.z[phant.nz] << "]).  Time elapsed is " << duration << " s.\n";


        progress->setValue(1000000000);
        progWin->hide();
        progWin->close();

    }

    //-----------------------------------------------------------------
    // Save egsphant file
    //-----------------------------------------------------------------
    phant.saveEGSPhantFile(egs_input_file->egsphant_location);

    //compress the egsphant file
    QProcess *qzip = new QProcess();
    qzip->setProcessChannelMode(QProcess::MergedChannels);
    qzip->setReadChannel(QProcess::StandardOutput);
    QString command = "gzip -f " + egs_input_file->egsphant_location;  //-f used if exisitng .gz file, -f forces the
    qzip->execute(command);


    duration = (std::clock()-start)/(double)CLOCKS_PER_SEC;
    std::cout << "Egsphant file successfully output." <<egs_input_file->egsphant_location.toStdString() <<".gz  Time elapsed is " << duration << " s.\n";

    //trimEGS->generatedPhant = true;

    continue_AT_egsinp();
    this->setEnabled(true);
}




/***
Function: setup_MAR
--------------------
Process: used to setup the variables of the Mar process
    uses this function such that it is easy to change values of the STR
***/
void Interface::setup_MAR() {
    this->setDisabled(true);
    STRwindow = new QWidget();

    title = new QLabel(tr("Verify the Simple Threshold Replacement Values"));
    low = new QLabel(tr("Low threshold [g cm <sup>-3</sup> ]"));
    high = new QLabel(tr("High threshold [g cm <sup>-3</sup> ]"));
    replacement_lab = new QLabel(tr("Replacement density [g cm <sup>-3</sup> ]"));
    QLabel *radius = new QLabel(tr("Radius of the cylindrical replacement region surrounding each seed [mm]"));
    QLabel *contour_lab = new QLabel(tr("Select the contour which contains the seeds (optional)"));
    low->setToolTip(tr("The low threshold represents the lower boundary density\n")+
                    tr("for voxels surrounding each seed"));
    high->setToolTip(tr("The high threshold represents the upper boundary density\n")+
                     tr("for voxels surrounding each seed"));
    replacement_lab->setToolTip(tr("The replacement density is used for all voxels\n")+
                                tr("whose density is outside the upper and lower boundary."));
    contour_lab->setToolTip(tr("Voxels below the low threshold within the contour\n")+
                            tr("will be replaced with the replacement value.\n"));
    low_value = new QLineEdit();
    high_value = new QLineEdit();
    replacement_value = new QLineEdit();
    radius_value = new QLineEdit();
    contour_list = new QComboBox();
    QVector<QString> contours = get_data->structName;
    contours.prepend(" ");
    contour_list->addItems(contours.toList());
    contour_list->setCurrentIndex(0);
    STRokay = new QPushButton("Ok");

    low_value->setText(QString::number(low_threshold));
    high_value->setText(QString::number(high_threshold));
    replacement_value->setText(QString::number(replacement));
    radius_value->setText(QString::number(xy_search_in_mm));

    options_layout = new QGridLayout();


    options_layout->addWidget(title, 0, 0, 1, 2);
    options_layout->addWidget(low, 1, 0, 1, 1);
    options_layout->addWidget(low_value, 1, 1, 1, 1);
    options_layout->addWidget(high, 2, 0, 1, 1);
    options_layout->addWidget(high_value, 2, 1, 1, 1);
    options_layout->addWidget(replacement_lab, 3, 0, 1, 1);
    options_layout->addWidget(replacement_value, 3, 1, 1, 1);
    options_layout->addWidget(radius, 4, 0, 1, 1);
    options_layout->addWidget(radius_value, 4, 1, 1, 1);
    options_layout->addWidget(contour_lab, 5, 0, 1, 1);
    options_layout->addWidget(contour_list, 5, 1, 1, 1);
    options_layout->addWidget(STRokay,  7, 1, 1, 1);

    options_layout->setColumnStretch(0, 2);
    options_layout->setColumnStretch(1, 1);


    STRwindow-> setLayout(options_layout); //this may be writing over the 'main' window


    connect(STRokay, SIGNAL(clicked()),
            this, SLOT(apply_str_to_seed_locations()));

    STRwindow->show();
    STRwindow->activateWindow();
    STRwindow->raise();

}



/***
Function: apply_str_to_seed_locations
-------------------------------------

    threshold: the threshold CT number to identify replacement is needed
    replacement: the replacement CT number
    xy_search_in_mm: the area (in mm) that STR is applied to around the seed
    z_search in slices: the number of z-slices (depth) that STR is applied to around the seed

Process:
    -Itterate through the seeds
    -Checks the voxels around a seed (seeds often cause a scattering of photons around them with is shown as a high density (bone/calcification) on CT)
    -If the density is above a threshold, it is replaced by a user specified replacement value


//Derived from Marc Chamberland's egs_tools/ct_tools/artifact.py code
//cube is defined about each seed, if the voxel CT number is above a treshold, it will be replaced with a lower number
//deering: EC_NB_v1 ha a different code, but still simple, uses cylMaxRadius and replaces with -90.2
***/
void Interface::apply_str_to_seed_locations() {
    //Check the values are appropriate
    bool oklow, okhigh, okrepl, okrad;
    double low =  QString(low_value->text()).toDouble(&oklow);
    double high =  QString(high_value->text()).toDouble(&okhigh);
    double repl_value =  QString(replacement_value->text()).toDouble(&okrepl);
    double radius =  QString(radius_value->text()).toDouble(&okrad);

    if (!oklow || low < 0) {
        STRwindow->setDisabled(true);
        QMessageBox msgBox;
        msgBox.setText(tr("The low threshold value is invalid"));
        msgBox.exec();

    }
    else if (!okhigh || high < 0) {
        STRwindow->setDisabled(true);
        QMessageBox msgBox;
        msgBox.setText(tr("The high threshold value is invalid"));
        msgBox.exec();

    }
    else if (!okrepl || repl_value < 0) {
        STRwindow->setDisabled(true);
        QMessageBox msgBox;
        msgBox.setText(tr("The replacement value is invalid"));
        msgBox.exec();

    }
    else if (!okrad || radius < 0) {
        STRwindow->setDisabled(true);
        QMessageBox msgBox;
        msgBox.setText(tr("The radius value is invalid"));
        msgBox.exec();

    }
    else {
        replacement = repl_value;
        high_threshold = high;
        xy_search_in_mm = radius;
        low_threshold = low;

        setup_MAR_Flag = true;
        STRwindow->hide();
        STRwindow->close();

        //Used for the low threshold replacement
        if (contour_list->currentIndex() > 0) {
            indexMARContour = contour_list->currentIndex() -1;
        }


        int z_search_in_slices=2;
        int count_values_replaced = 0;
        double distance_from_seed;
        double xy_search_in_cm = xy_search_in_mm/10.;

        int xy_search_in_voxels = ceil(xy_search_in_cm/fabs(phant.x[0]-phant.x[1]));

        for (int i=0; i<get_data->all_seed_pos.size(); i++) {
            for (int j=0; j<get_data->all_seed_pos[i].size(); j++) {

                QVector <int> ijk = get_ijk_from_xyz(get_data->all_seed_pos[i][j].x, get_data->all_seed_pos[i][j].y, get_data->all_seed_pos[i][j].z);   //the seed location
                if (ijk[0] == -1 || ijk[1] == -1 || ijk[2] == -1) {
                    std::cout<<"ERROR: unable to apply STR to seed location. Seed index is outside of the phantom (x,y,z): (" <<get_data->all_seed_pos[i][j].x <<", " <<get_data->all_seed_pos[i][j].y <<", " <<get_data->all_seed_pos[i][j].z <<") \n";
                }
                else {

                    for (int z = -z_search_in_slices; z < (z_search_in_slices+1); z++) {
                        for (int y=-xy_search_in_voxels; y<= xy_search_in_voxels; y++) {
                            for (int x=-xy_search_in_voxels; x<= xy_search_in_voxels; x++) {

                                QVector<double> xyz = get_voxel_center_from_ijk(ijk[0]+x,ijk[1]+y,ijk[2] +z);

                                distance_from_seed = sqrt(pow(fabs(get_data->all_seed_pos[i][j].x - xyz[0]),2) + pow(fabs(get_data->all_seed_pos[i][j].y - xyz[1]),2));

                                if (distance_from_seed < xy_search_in_cm) {

                                    int tempHU = get_data->HU[ijk[2] +z][ijk[1]+y][ijk[0]+x];
                                    int n=0;

                                    for (n = 0; n < HUMap.size()-1; n++)
                                        if (HUMap[n] <= tempHU && tempHU < HUMap[n+1]) {
                                            break;
                                        }


                                    if (tempHU < HUMap[0]) {
                                        n = 0;
                                    }
                                    if (tempHU > HUMap[HUMap.size()-1]) {
                                        n = HUMap.size() -2;
                                    }

                                    double temp_density = interp(tempHU,HUMap[n],HUMap[n+1],denMap[n],denMap[n+1]);

                                    if (temp_density > high_threshold || temp_density < low_threshold) {
                                        phant.d[ijk[0]+x][ijk[1]+y][ijk[2] +z] = replacement;
                                        count_values_replaced++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        std::cout<<"Applied STR. " <<count_values_replaced <<" values replaced. \n";
        if (extraOptionsFlag) {
            ATcreate_egsphant();    //If in the extra options tab, use this function
        }
        else {
            create_egsphant();
        }
    }
    STRwindow->setEnabled(true);
}






//--------------------------------------------------------------------------
//Checks if the x,y,z cordinate is within the phantom
//Assumes baounds are in order of small to large
bool Interface::inPhantom(double x, double y, double z) {

    if (x< phant.x[0] || x> phant.x[phant.nx]) {
        return false;
    }
    else if (y< phant.y[0] || y> phant.y[phant.ny]) {
        return false;
    }
    else if (z< phant.z[0] || z> phant.x[phant.nz]) {
        return false;
    }
    else {
        return true;
    }

}


int Interface::get_idx_from_bounds(double pos, QVector<double> bounds)
// get_idx_from_bounds returns the 1 dimensional index of the voxel
// whose bounds contain pos
{
    //point is outside of all bounds
    if ((pos < bounds[0]) || (bounds.back() < pos)) {
        return -1;
    }

    //check end cases
    if (pos == bounds[0]) {
        return 0;
    }
    if (pos == bounds.back()) {
        return bounds.size() - 1;
    }

    //normal case
    int idx = 0;
    while (pos>bounds[idx]) {
        idx++;
    }
    return idx-1;
}


QVector<int> Interface::get_ijk_from_xyz(double x, double y, double z)
// get_ijk_from_xyz takes in x,y,z and returns the (i,j,k) of the dose dist
{
    int ii = get_idx_from_bounds(x,phant.x);
    int jj = get_idx_from_bounds(y,phant.y);
    int kk = get_idx_from_bounds(z,phant.z);

    QVector<int> ijk;
    ijk.push_back(ii);
    ijk.push_back(jj);
    ijk.push_back(kk);

    return ijk;
}


//--------------------------------------------------
//--------------------------------------------------
//      Converting 3ddose <-> DICOM dose
//--------------------------------------------------
//--------------------------------------------------
/***

***/

/***
Function: read_3ddose
----------------------
Process: 3ddose to DICOM dose
          Reads the 3ddose file created by egs_brachy and creates the DICOM dose file

***/
void Interface::read_3ddose() {
    QTextStream out(stdout);

    this->setDisabled(true);
    QMessageBox msgBox;
    msgBox.setText(tr("Select the 3ddose file."));
    msgBox.setWindowTitle(tr("egs_brachy GUI"));
    msgBox.exec();

    QString path = QFileDialog::getOpenFileName(
                       this,
                       tr("Select the 3ddose file to open"),
                       egs_brachy_home_path,
                       "3ddose (*.3ddose)");

    if (!path.isEmpty()) {
        //User selects location and file name of dicom dose file
        //------------------------------------------------------
        QString path2 = QFileDialog::getSaveFileName(0, "Save the DICOM Dose file",
                        egs_brachy_home_path, "DICOM (*.dcm)");

        if (path2.isEmpty()) {
            std::cout << "DICOM Dose file not be created \n";
        }
        else if (!path2.isEmpty()) {

            dose = new read_dose;
            dose->load_dose_data(path); //read the 3ddose file

            QMap<QString, QString> emptyvect;   //used in following function to show there was
            //no data extracted from previous dicom files (ie creation dat, patient name...)

            dose->create_dicom_dose(emptyvect, path2); //format and output the DICOM dose file

            //User can view 'simple' metrics
            QMessageBox msgBox;
            msgBox.setText(tr("Created the DICOM dose file. \nWould you like to view the metrics?"));
            msgBox.setWindowTitle(tr("egs_brachy GUI"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();

            switch (ret) {
            case QMessageBox::No:
                // Do nothing
                break;
            case QMessageBox::Yes: {
                // Show the metrics
                calc_metrics_3ddose = new metrics;
                //calculating the metrics
                QVector <QVector <QVector <int> > >empty;
                QMap <int, QString> empty_string;
                calc_metrics_3ddose->get_data(dose->x, dose->y, dose->z, dose->cx, dose->cy, dose->cz, empty,
                                              dose->val, dose->err, empty_string);

                //connect(calc_metrics_3ddose, SIGNAL(closed()), this, SLOT(closed_metrics()));


                calc_metrics_3ddose->window->show();
                QApplication::processEvents();
                calc_metrics_3ddose->setEnabled(TRUE);

                break;
            }
            default:
                break;  // should never be reached
            }
        }

        this->setEnabled(true);
    }
    this->setEnabled(true);
}


/***
Function: launch_dose_to_3ddose
-------------------------------
Process: Retrieves the dicom dose file from the user and creates the 3ddose file

***/
void Interface::launch_dose_to_3ddose() {
    this->setDisabled(true);
    QMessageBox msgBox;
    msgBox.setText(tr("Select the location of the DICOM dose file."));
    msgBox.setWindowTitle(tr("egs_brachy GUI"));
    msgBox.exec();

    QString dose_file = QFileDialog::getOpenFileName(
                            this,
                            tr("Select the DICOM dose file"),
                            "/data/data060/sjarvis/",
                            "dicom (*.dcm)");

    if (!dose_file.isEmpty()) {
        QString save_3ddose_file = QFileDialog::getSaveFileName(
                                       this,
                                       tr("Save the 3ddose file"),
                                       egs_brachy_home_path,
                                       "3ddose (*.3ddose)");

        if (!save_3ddose_file.isEmpty()) {

            this->setDisabled(true);
            get_data->create_3ddose(dose_file, save_3ddose_file);
            this->setEnabled(true);
        }
    }

    this->setEnabled(true);

}


/***
Function: create_3ddose
------------------------
Input: file_path: the path of a RT Dose file
Process:

Format of 3ddose --
Row/Block 1: number of voxels in x,y,z directions (e.g., nx, ny, nz)
Row/Block 2: voxel boundaries (cm) in x direction(nx +1 values)
Row/Block 3: voxel boundaries (cm) in y direction (ny +1 values)
Row/Block 4: voxel boundaries (cm) in z direction(nz +1 values)
Row/Block 5: dose values array (nx.ny.nz values)
Row/Block 6: error values array (fractional errors, nx.ny.nz values)

***/
void DICOM::create_3ddose(QString file_path, QString path_3ddose) {

    //create_progress_bar_dicom();
    //setup_progress_bar_dicom("Parsing DICOM Dose File and creating .3ddose file", "");
    //double increment = 1000000000/15;

    //updateProgress(increment);

    QVector <double> imagePos;
    // int numFrames; // unused
    unsigned short int xPix;
    unsigned short int yPix;
    QVector <double> xySpacing;
    int bits = 0;
    double doseScaling = 1;
    QVector <double> frame_offset;
    QVector <double> DOSE;
    // int rescaleFlag = 0; // unused
    QVector<double> xbounds;
    QVector<double> ybounds;
    QVector<double> zbounds;
    QVector<QVector <double>> doseVals;
    QVector<QVector <double>> volVals;
    QVector <Attribute *> *att;

    QStringList pointData;
    database dat;
    DICOM *d = new DICOM(&dat);
    DICOM *dicomDose;

    if (d->parse(file_path)) {
        dicomDose = d;
        for (int j = 0; j < dicomDose->data.size(); j++) {
            if (dicomDose->data[j]->tag[0] == 0x0008 && dicomDose->data[j]->tag[1] == 0x0060) {
                QString temp = "";
                for (unsigned int s = 0; s < dicomDose->data[j]->vl; s++) {
                    temp.append(dicomDose->data[j]->vf[s]);
                }

                temp = temp.trimmed();

                if (temp !="RTDOSE") {
                    std::cout<<"The dicom file (" <<file_path.toStdString() <<") is not a dose file \n";
                    return;
                }
            }
        }

        //updateProgress(increment);
        //Parse the dose file extracting all the relevant data/tags
        for (int j = 0; j < dicomDose->data.size(); j++) {
            if (dicomDose->data[j]->tag[0] == 0x0020 && dicomDose->data[j]->tag[1] == 0x0032) { //Image Position Patient

                QString temp = "";
                for (unsigned int s = 0; s < dicomDose->data[j]->vl; s++) {
                    temp.append(dicomDose->data[j]->vf[s]);
                }
                imagePos.resize(3);
                imagePos[0] = (temp.split('\\',QString::SkipEmptyParts)[0]).toDouble();
                imagePos[1] = (temp.split('\\',QString::SkipEmptyParts)[1]).toDouble();
                imagePos[2] = (temp.split('\\',QString::SkipEmptyParts)[2]).toDouble();

            }
            //else if (dicomDose->data[j]->tag[0] == 0x0028 && dicomDose->data[j]->tag[1] == 0x0008) {   //Number of Frames // unused
			//                                                                                                              // unused
            //    QString temp = "";                                                                                        // unused
            //    for (unsigned int s = 0; s < dicomDose->data[j]->vl; s++) {                                               // unused
            //        temp.append(dicomDose->data[j]->vf[s]);                                                               // unused
            //    }                                                                                                         // unused
            //    numFrames = temp.toInt();                                                                                 // unused
			//                                                                                                              // unused
            //}                                                                                                             // unused
            else if (dicomDose->data[j]->tag[0] == 0x0028 && dicomDose->data[j]->tag[1] == 0x0010) {   //Number of Rows

                if (dicomDose->isBigEndian)
                    yPix = (unsigned short int)(((short int)(dicomDose->data[j]->vf[0]) << 8) +
                                                (short int)(dicomDose->data[j]->vf[1]));
                else
                    yPix = (unsigned short int)(((short int)(dicomDose->data[j]->vf[1]) << 8) +
                                                (short int)(dicomDose->data[j]->vf[0]));

            }
            else if (dicomDose->data[j]->tag[0] == 0x0028 && dicomDose->data[j]->tag[1] == 0x0011) {   //Number of Columns

                if (dicomDose->isBigEndian)
                    xPix = (unsigned short int)(((short int)(dicomDose->data[j]->vf[0]) << 8) +
                                                (short int)(dicomDose->data[j]->vf[1]));
                else
                    xPix = (unsigned short int)(((short int)(dicomDose->data[j]->vf[1]) << 8) +
                                                (short int)(dicomDose->data[j]->vf[0]));
            }
            else if (dicomDose->data[j]->tag[0] == 0x0028 && dicomDose->data[j]->tag[1] == 0x0030) {   //Pixel Spacing

                QString temp = "";
                for (unsigned int s = 0; s < dicomDose->data[j]->vl; s++) {
                    temp.append(dicomDose->data[j]->vf[s]);
                }
                xySpacing.resize(2);
                xySpacing[0] = (temp.split('\\',QString::SkipEmptyParts)[0]).toDouble();
                xySpacing[1] = (temp.split('\\',QString::SkipEmptyParts)[1]).toDouble();

            }
            else if (dicomDose->data[j]->tag[0] == 0x0028 && dicomDose->data[j]->tag[1] == 0x0100) {   //Bits Allocated

                if (dicomDose->isBigEndian)
                    bits = (unsigned short int)(((short int)(dicomDose->data[j]->vf[0]) << 8) +
                                                (short int)(dicomDose->data[j]->vf[1]));
                else
                    bits = (unsigned short int)(((short int)(dicomDose->data[j]->vf[1]) << 8) +
                                                (short int)(dicomDose->data[j]->vf[0]));

            }
            else if (dicomDose->data[j]->tag[0] == 0x3004 && dicomDose->data[j]->tag[1] == 0x000c) {   //Grid Frame Offset

                QString temp = "";
                for (unsigned int s = 0; s < dicomDose->data[j]->vl; s++) {
                    temp.append(dicomDose->data[j]->vf[s]);
                }
                //Long vector - do something like this
                pointData = temp.split('\\');
                for (int m = 0; m < pointData.size(); m++) {
                    frame_offset.append(pointData[m].toDouble());
                }

            }
            else if (dicomDose->data[j]->tag[0] == 0x3004 && dicomDose->data[j]->tag[1] == 0x000e) {   //Dose Grid Scaling

                QString temp = "";
                for (unsigned int s = 0; s < dicomDose->data[j]->vl; s++) {
                    temp.append(dicomDose->data[j]->vf[s]);
                }
                doseScaling = temp.toDouble();
            }
            else if (dicomDose->data[j]->tag[0] == 0x7fe0 && dicomDose->data[j]->tag[1] == 0x0010) {    //Pixel Data

                if (bits == 32) {
                    int temp;

                    if (dicomDose->isBigEndian)
                        for (unsigned int s = 0; s < dicomDose->data[j]->vl; s+=4) {

                            temp  = (dicomDose->data[j]->vf[s+1]);
                            temp += (short int)(dicomDose->data[j]->vf[s]) << 8;
                            temp += (short int)(dicomDose->data[j]->vf[s+3]) << 16;
                            temp += (short int)(dicomDose->data[j]->vf[s+2]) << 24;

                            DOSE.append(doseScaling*temp);
                        }
                    else
                        for (unsigned int s = 0; s < dicomDose->data[j]->vl; s+=4) {
                            temp  = (dicomDose->data[j]->vf[s]);
                            temp += (short int)(dicomDose->data[j]->vf[s+1]) << 8;
                            temp += (short int)(dicomDose->data[j]->vf[s+2]) << 16;
                            temp += (short int)(dicomDose->data[j]->vf[s+3]) << 24;

                            DOSE.append(doseScaling*temp);
                        }
                }
                else {
                    short int temp;

                    if (dicomDose->isBigEndian)
                        for (unsigned int s = 0; s < dicomDose->data[j]->vl; s+=2) {
                            temp  = (dicomDose->data[j]->vf[s+1]);
                            temp += (short int)(dicomDose->data[j]->vf[s]) << 8;

                            DOSE.append(doseScaling*temp);
                        }
                    else
                        for (unsigned int s = 0; s < dicomDose->data[j]->vl; s+=2) {
                            temp  = (dicomDose->data[j]->vf[s]);
                            temp += (short int)(dicomDose->data[j]->vf[s+1]) << 8;

                            DOSE.append(doseScaling*temp);
                        }

                }

            }
            else if (dicomDose->data[j]->tag[0] == 0x3004 && dicomDose->data[j]->tag[1] == 0x0050) {   //DVH Sequence
                QByteArray tempData;
                for (int k = 0; k < dicomDose->data[j]->seq.items.size(); k++) {
                    tempData = QByteArray((char *)dicomDose->data[j]->seq.items[k]->vf,dicomDose->data[j]->seq.items[k]->vl);
                    QDataStream dataStream(tempData);

                    att = new QVector <Attribute *>;
                    if (!dicomDose->parseSequence(&dataStream, att)) {
                        std::cout << "Failed to parse sequence data for tag (3004,0050)  \n";
                    }

                    for (int l = 0; l < att->size(); l++)
                        if (att->at(l)->tag[0] == 0x3004 && att->at(l)->tag[1] == 0x0058) {
                            QStringList DVHdata;
                            doseVals.resize(doseVals.size()+1);
                            volVals.resize(volVals.size()+1);
                            QString tempS = "";

                            for (unsigned int s = 0; s < att->at(l)->vl; s++) {
                                tempS.append(att->at(l)->vf[s]);
                            }

                            DVHdata = tempS.split('\\');

                            for (int m = 0; m < DVHdata.size(); m+=2) {
                                doseVals.last().append(DVHdata[m].toDouble());
                                volVals.last().append(DVHdata[m+1].toDouble());
                            }
                        }


                    for (int l = 0; l < att->size(); l++) {
                        delete att->at(l);
                    }
                    delete att;
                }
            }
            //updateProgress(increment);
        }

        // QString fileprofilepath("/home/sjarvis/DVHdata.txt");
        // QFile fileprofile(fileprofilepath);

        // if (fileprofile.open(QIODevice::WriteOnly | QIODevice::Text))    //Creating the code for the image
        // {
        // QTextStream output(&fileprofile);
        // for(int i=0; i<doseVals.size(); i++){
        // output<<"Dose val " <<i <<"\n";
        // for(int j=0; j<doseVals[i].size(); j++)
        // output<<volVals[i][j] <<"\t" <<doseVals[i][j] <<"\n";
        // }

        // fileprofile.close();
        // }

        std::cout<<"Parsed " <<file_path.toStdString() <<" \n";

        double xthick = xySpacing[0]/10.0;
        double ythick = xySpacing[1]/10.0;
        double zthick = frame_offset[1]/10.0 - frame_offset[0]/10.0;
        double zstart= imagePos[2]/10.0;
        //updateProgress(increment);

        //x
        for (int i=0; i<xPix; i++) { // defining the xbounds in the CTdata object
            xbounds.push_back(imagePos[0]/10 + xthick*i - xthick/2);
        }
        xbounds.push_back(xbounds.back()+xthick);
        //updateProgress(increment);

        //y
        for (int i=0; i<yPix; i++) { // defining the ybounds in the CTdata object
            ybounds.push_back(imagePos[1]/10  + ythick*i- ythick/2);
        }
        ybounds.push_back(ybounds.back()+ythick);
        //updateProgress(increment);

        //z
        if (frame_offset[0] == 0) { // relative coordinates, add the image position to get the values
            for (int i=0; i<frame_offset.size(); i++) { // defining the xbounds in the CTdata object
                zbounds.push_back(frame_offset[i]/10.0 + zstart - zthick/2);
            }

        }
        else {      //absolute coordinates
            for (int i=0; i<frame_offset.size(); i++) { // defining the xbounds in the CTdata object
                zbounds.push_back(frame_offset[i]/10.0);
            }

        }

        //updateProgress(increment);
        //create the 3ddose file
        std::ofstream dose_file;
        dose_file.open(path_3ddose.toStdString().c_str());

        //Output the egsphant to a file
        //-----------------------------------
        if (dose_file.is_open()) {

            //updateProgress(increment);
            //1: Output the number of voxels in each dimension
            dose_file << xPix << "   " << yPix << "   " << frame_offset.size() <<"\n";

            //updateProgress(increment);

            //2,3,4: Output the voxel boundaries in the x,y,z direction
            for (int i=0; i< xbounds.size(); i++) {
                dose_file<<" " <<xbounds[i];
            }

            dose_file <<"\n";

            //updateProgress(increment);
            for (int i=0; i<ybounds.size(); i++) {
                dose_file<<" " <<ybounds[i];
            }

            dose_file<<"\n";

            //updateProgress(increment);
            for (int i=0; i<zbounds.size(); i++) {
                dose_file<<" " <<zbounds[i];
            }

            dose_file<<"\n";

            //updateProgress(increment);
            //5: Dose value array
            for (int i=0; i<DOSE.size(); i++) {
                dose_file<<"	" <<DOSE[i];
            }

            //updateProgress(increment);
            dose_file<<"\n";

            dose_file.close();

            std::cout<<"Successfully created the 3ddose file. Location: " <<path_3ddose.toStdString() <<"\n";
            //updateProgress(increment);

        }
        else {
            std::cout<<"ERROR: Couldn't open 3ddose file location \n";
        }

        //updateProgress(increment);
        //progress2->setValue(1000000000);

    }
    else {
        std::cout<<"Unable to parse " <<file_path.toStdString() <<"\n";
    }

}


QVector<double> Interface::get_voxel_center_from_ijk(double ii, double jj, double kk)
// get_voxel_center_from_idx takes an index and returns the coordinates
// of the center of the voxel

{
    QVector<double> vox_cent;

    vox_cent.push_back((phant.x[ii]+phant.x[ii+1])/2);
    vox_cent.push_back((phant.y[jj]+phant.y[jj+1])/2);
    vox_cent.push_back((phant.z[kk]+phant.z[kk+1])/2);

    return vox_cent;
}


QString Interface::stat(double min, double max, double nBin, QVector<double> diff) {
    // Build a QString that holds csv data
    QString output = "";
    QString delimiter = ",";
    QVector <double> range;
    QVector <int> freq;
    for (int i = 0; i < nBin; i++) {
        range += min + ((max-min)/double(nBin) * i);
        freq += 0;
    }
    range += max;
    int nVox = 0;
    // Additional values defaults
    //*nVox = *totVol = *rms = *chi = *area1 = *area2 = 0;

    // Iterate through every voxel to find the number of DVHpoints needed
    // for either the whole thing, a region or several media

    for (int n = 0; n < diff.size(); n++) {
        if (diff[n] < min) // If the dose is under our
        { /*Do nothing*/ }      // minimum value, then do nothing
        if (diff[n]> max) // maximum value, then do nothing
        { /*Do nothing*/ }
        else {
            // Iterate through every bin, and if the dose is
            // less than the upper limit of bin p, then
            // add 1 to the frequency of that bin, add its
            // volume to the total, count the voxel, add to
            // RMS and add to Chi-Squared
            for (int p = 0; p < nBin; p++)
                if (diff[n] < range[p+1]) {
                    freq[p]++;
                    (nVox)++;
                    break; // stop iterating higher bins
                }
        }
    }


    // output += "bin" + delimiter + "count \n";
    // output += "Total number of voxels " + QString::number(nVox) + "\n";
    // Make the histogram output
    for (int p = 0; p < range.size()-1; p++) {
        //output += QString::number((range[p]+range[p+1])/2.0, 'E', 8);
        output += "\t";

        output += QString::number(range[p+1], 'E', 8);
        output += "\t";

        // We take the fraction of the total and turn it into a percentage
        output += QString::number(freq[p], 'E', 8);
        output += "\n";

    }
    return output;
}
