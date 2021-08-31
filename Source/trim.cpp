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

#include "trim.h"

/***

This class creates the allows the user to trim the egsphant and inscribe the phantom within a geometry/an geometry within the phant
by allowing the user to select the file path

***/

//Class obtained from https://github.com/mcallegari/qlcplus/blob/master/ui/src/ctkrangeslider.cpp
class ctkRangeSliderPrivate {
    Q_DECLARE_PUBLIC(ctkRangeSlider);
protected:
    ctkRangeSlider *const q_ptr;
public:
    /// Boolean indicates the selected handle
    ///   True for the minimum range handle, false for the maximum range handle
    enum Handle {
        NoHandle = 0x0,
        MinimumHandle = 0x1,
        MaximumHandle = 0x2
    };
    Q_DECLARE_FLAGS(Handles, Handle);

    ctkRangeSliderPrivate(ctkRangeSlider &object);
    void init();

    /// Return the handle at the given pos, or none if no handle is at the pos.
    /// If a handle is selected, handleRect is set to the handle rect.
    /// otherwise return NoHandle and handleRect is set to the combined rect of
    /// the min and max handles
    Handle handleAtPos(const QPoint &pos, QRect &handleRect)const;

    /// Copied verbatim from QSliderPrivate class (see QSlider.cpp)
    int pixelPosToRangeValue(int pos) const;
    int pixelPosFromRangeValue(int val) const;

    /// Draw the bottom and top sliders.
    void drawMinimumSlider(QStylePainter *painter) const;
    void drawMaximumSlider(QStylePainter *painter) const;

    /// End points of the range on the Model
    int m_MaximumValue;
    int m_MinimumValue;

    /// End points of the range on the GUI. This is synced with the model.
    int m_MaximumPosition;
    int m_MinimumPosition;

    /// Controls selected ?
    QStyle::SubControl m_MinimumSliderSelected;
    QStyle::SubControl m_MaximumSliderSelected;

    /// See QSliderPrivate::clickOffset.
    /// Overrides this ivar
    int m_SubclassClickOffset;

    /// See QSliderPrivate::position
    /// Overrides this ivar.
    int m_SubclassPosition;

    /// Original width between the 2 bounds before any moves
    int m_SubclassWidth;

    ctkRangeSliderPrivate::Handles m_SelectedHandles;

    /// When symmetricMoves is true, moving a handle will move the other handle
    /// symmetrically, otherwise the handles are independent.
    bool m_SymmetricMoves;

    QString m_HandleToolTip;

private:
    Q_DISABLE_COPY(ctkRangeSliderPrivate);
};

// --------------------------------------------------------------------------
ctkRangeSliderPrivate::ctkRangeSliderPrivate(ctkRangeSlider &object)
    :q_ptr(&object) {
    this->m_MinimumValue = 0;
    this->m_MaximumValue = 100;
    this->m_MinimumPosition = 0;
    this->m_MaximumPosition = 100;
    this->m_MinimumSliderSelected = QStyle::SC_None;
    this->m_MaximumSliderSelected = QStyle::SC_None;
    this->m_SubclassClickOffset = 0;
    this->m_SubclassPosition = 0;
    this->m_SubclassWidth = 0;
    this->m_SelectedHandles = NoHandle;
    this->m_SymmetricMoves = false;
}

// --------------------------------------------------------------------------
void ctkRangeSliderPrivate::init() {
    Q_Q(ctkRangeSlider);
    this->m_MinimumValue = q->minimum();
    this->m_MaximumValue = q->maximum();
    this->m_MinimumPosition = q->minimum();
    this->m_MaximumPosition = q->maximum();
    q->connect(q, SIGNAL(rangeChanged(int,int)), q, SLOT(onRangeChanged(int,int)));
}

// --------------------------------------------------------------------------
ctkRangeSliderPrivate::Handle ctkRangeSliderPrivate::handleAtPos(const QPoint &pos, QRect &handleRect)const {
    Q_Q(const ctkRangeSlider);

    QStyleOptionSlider option;
    q->initStyleOption(&option);

    // The functinos hitTestComplexControl only know about 1 handle. As we have
    // 2, we change the position of the handle and test if the pos correspond to
    // any of the 2 positions.

    // Test the MinimumHandle
    option.sliderPosition = this->m_MinimumPosition;
    option.sliderValue    = this->m_MinimumValue;

    QStyle::SubControl minimumControl = q->style()->hitTestComplexControl(
                                            QStyle::CC_Slider, &option, pos, q);
    QRect minimumHandleRect = q->style()->subControlRect(
                                  QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, q);

    // Test if the pos is under the Maximum handle
    option.sliderPosition = this->m_MaximumPosition;
    option.sliderValue    = this->m_MaximumValue;

    QStyle::SubControl maximumControl = q->style()->hitTestComplexControl(
                                            QStyle::CC_Slider, &option, pos, q);
    QRect maximumHandleRect = q->style()->subControlRect(
                                  QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, q);

    // The pos is above both handles, select the closest handle
    if (minimumControl == QStyle::SC_SliderHandle &&
            maximumControl == QStyle::SC_SliderHandle) {
        int minDist = 0;
        int maxDist = 0;
        if (q->orientation() == Qt::Horizontal) {
            minDist = pos.x() - minimumHandleRect.left();
            maxDist = maximumHandleRect.right() - pos.x();
        }
        else { //if (q->orientation() == Qt::Vertical)
            minDist = minimumHandleRect.bottom() - pos.y();
            maxDist = pos.y() - maximumHandleRect.top();
        }
        Q_ASSERT(minDist >= 0 && maxDist >= 0);
        minimumControl = minDist < maxDist ? minimumControl : QStyle::SC_None;
    }

    if (minimumControl == QStyle::SC_SliderHandle) {
        handleRect = minimumHandleRect;
        return MinimumHandle;
    }
    else if (maximumControl == QStyle::SC_SliderHandle) {
        handleRect = maximumHandleRect;
        return MaximumHandle;
    }
    handleRect = minimumHandleRect.united(maximumHandleRect);
    return NoHandle;
}

// --------------------------------------------------------------------------
// Copied verbatim from QSliderPrivate::pixelPosToRangeValue. See QSlider.cpp
//
int ctkRangeSliderPrivate::pixelPosToRangeValue(int pos) const {
    Q_Q(const ctkRangeSlider);
    QStyleOptionSlider option;
    q->initStyleOption(&option);

    QRect gr = q->style()->subControlRect(QStyle::CC_Slider,
                                          &option,
                                          QStyle::SC_SliderGroove,
                                          q);
    QRect sr = q->style()->subControlRect(QStyle::CC_Slider,
                                          &option,
                                          QStyle::SC_SliderHandle,
                                          q);
    int sliderMin, sliderMax, sliderLength;
    if (option.orientation == Qt::Horizontal) {
        sliderLength = sr.width();
        sliderMin = gr.x();
        sliderMax = gr.right() - sliderLength + 1;
    }
    else {
        sliderLength = sr.height();
        sliderMin = gr.y();
        sliderMax = gr.bottom() - sliderLength + 1;
    }

    return QStyle::sliderValueFromPosition(q->minimum(),
                                           q->maximum(),
                                           pos - sliderMin,
                                           sliderMax - sliderMin,
                                           option.upsideDown);
}

//---------------------------------------------------------------------------
int ctkRangeSliderPrivate::pixelPosFromRangeValue(int val) const {
    Q_Q(const ctkRangeSlider);
    QStyleOptionSlider option;
    q->initStyleOption(&option);

    QRect gr = q->style()->subControlRect(QStyle::CC_Slider,
                                          &option,
                                          QStyle::SC_SliderGroove,
                                          q);
    QRect sr = q->style()->subControlRect(QStyle::CC_Slider,
                                          &option,
                                          QStyle::SC_SliderHandle,
                                          q);
    int sliderMin, sliderMax, sliderLength;
    if (option.orientation == Qt::Horizontal) {
        sliderLength = sr.width();
        sliderMin = gr.x();
        sliderMax = gr.right() - sliderLength + 1;
    }
    else {
        sliderLength = sr.height();
        sliderMin = gr.y();
        sliderMax = gr.bottom() - sliderLength + 1;
    }

    return QStyle::sliderPositionFromValue(q->minimum(),
                                           q->maximum(),
                                           val,
                                           sliderMax - sliderMin,
                                           option.upsideDown) + sliderMin;
}

//---------------------------------------------------------------------------
// Draw slider at the bottom end of the range
void ctkRangeSliderPrivate::drawMinimumSlider(QStylePainter *painter) const {
    Q_Q(const ctkRangeSlider);
    QStyleOptionSlider option;
    q->initMinimumSliderStyleOption(&option);

    option.subControls = QStyle::SC_SliderHandle;
    option.sliderValue = m_MinimumValue;
    option.sliderPosition = m_MinimumPosition;
    if (q->isMinimumSliderDown()) {
        option.activeSubControls = QStyle::SC_SliderHandle;
        option.state |= QStyle::State_Sunken;
    }
#ifdef Q_OS_MAC
    // On mac style, drawing just the handle actually draws also the groove.
    QRect clip = q->style()->subControlRect(QStyle::CC_Slider, &option,
                                            QStyle::SC_SliderHandle, q);
    painter->setClipRect(clip);
#endif
    painter->drawComplexControl(QStyle::CC_Slider, option);
}

//---------------------------------------------------------------------------
// Draw slider at the top end of the range
void ctkRangeSliderPrivate::drawMaximumSlider(QStylePainter *painter) const {
    Q_Q(const ctkRangeSlider);
    QStyleOptionSlider option;
    q->initMaximumSliderStyleOption(&option);

    option.subControls = QStyle::SC_SliderHandle;
    option.sliderValue = m_MaximumValue;
    option.sliderPosition = m_MaximumPosition;
    if (q->isMaximumSliderDown()) {
        option.activeSubControls = QStyle::SC_SliderHandle;
        option.state |= QStyle::State_Sunken;
    }
#ifdef Q_OS_MAC
    // On mac style, drawing just the handle actually draws also the groove.
    QRect clip = q->style()->subControlRect(QStyle::CC_Slider, &option,
                                            QStyle::SC_SliderHandle, q);
    painter->setClipRect(clip);
#endif
    painter->drawComplexControl(QStyle::CC_Slider, option);
}

// --------------------------------------------------------------------------
ctkRangeSlider::ctkRangeSlider(QWidget *_parent)
    : QSlider(_parent)
    , d_ptr(new ctkRangeSliderPrivate(*this)) {
    Q_D(ctkRangeSlider);
    d->init();
}

// --------------------------------------------------------------------------
ctkRangeSlider::ctkRangeSlider(Qt::Orientation o,
                               QWidget *parentObject)
    :QSlider(o, parentObject)
    , d_ptr(new ctkRangeSliderPrivate(*this)) {
    Q_D(ctkRangeSlider);
    d->init();
}

// --------------------------------------------------------------------------
ctkRangeSlider::ctkRangeSlider(ctkRangeSliderPrivate *impl, QWidget *_parent)
    : QSlider(_parent)
    , d_ptr(impl) {
    Q_D(ctkRangeSlider);
    d->init();
}

// --------------------------------------------------------------------------
ctkRangeSlider::ctkRangeSlider(ctkRangeSliderPrivate *impl, Qt::Orientation o,
                               QWidget *parentObject)
    :QSlider(o, parentObject)
    , d_ptr(impl) {
    Q_D(ctkRangeSlider);
    d->init();
}

// --------------------------------------------------------------------------
ctkRangeSlider::~ctkRangeSlider() {
}

// --------------------------------------------------------------------------
int ctkRangeSlider::minimumValue() const {
    Q_D(const ctkRangeSlider);
    return d->m_MinimumValue;
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setMinimumValue(int min) {
    Q_D(ctkRangeSlider);
    this->setValues(min, qMax(d->m_MaximumValue,min));
}

// --------------------------------------------------------------------------
int ctkRangeSlider::maximumValue() const {
    Q_D(const ctkRangeSlider);
    return d->m_MaximumValue;
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setMaximumValue(int max) {
    Q_D(ctkRangeSlider);
    this->setValues(qMin(d->m_MinimumValue, max), max);
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setValues(int l, int u) {
    Q_D(ctkRangeSlider);
    const int minValue =
        qBound(this->minimum(), qMin(l,u), this->maximum());
    const int maxValue =
        qBound(this->minimum(), qMax(l,u), this->maximum());
    bool emitMinValChanged = (minValue != d->m_MinimumValue);
    bool emitMaxValChanged = (maxValue != d->m_MaximumValue);

    d->m_MinimumValue = minValue;
    d->m_MaximumValue = maxValue;

    bool emitMinPosChanged =
        (minValue != d->m_MinimumPosition);
    bool emitMaxPosChanged =
        (maxValue != d->m_MaximumPosition);
    d->m_MinimumPosition = minValue;
    d->m_MaximumPosition = maxValue;

    if (isSliderDown()) {
        if (emitMinPosChanged || emitMaxPosChanged) {
            emit positionsChanged(d->m_MinimumPosition, d->m_MaximumPosition);
        }
        if (emitMinPosChanged) {
            emit minimumPositionChanged(d->m_MinimumPosition);
        }
        if (emitMaxPosChanged) {
            emit maximumPositionChanged(d->m_MaximumPosition);
        }
    }
    if (emitMinValChanged || emitMaxValChanged) {
        emit valuesChanged(d->m_MinimumValue,
                           d->m_MaximumValue);
    }
    if (emitMinValChanged) {
        emit minimumValueChanged(d->m_MinimumValue);
    }
    if (emitMaxValChanged) {
        emit maximumValueChanged(d->m_MaximumValue);
    }
    if (emitMinPosChanged || emitMaxPosChanged ||
            emitMinValChanged || emitMaxValChanged) {
        this->update();
    }
}

// --------------------------------------------------------------------------
int ctkRangeSlider::minimumPosition() const {
    Q_D(const ctkRangeSlider);
    return d->m_MinimumPosition;
}

// --------------------------------------------------------------------------
int ctkRangeSlider::maximumPosition() const {
    Q_D(const ctkRangeSlider);
    return d->m_MaximumPosition;
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setMinimumPosition(int l) {
    Q_D(const ctkRangeSlider);
    this->setPositions(l, qMax(l, d->m_MaximumPosition));
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setMaximumPosition(int u) {
    Q_D(const ctkRangeSlider);
    this->setPositions(qMin(d->m_MinimumPosition, u), u);
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setPositions(int min, int max) {
    Q_D(ctkRangeSlider);
    const int minPosition =
        qBound(this->minimum(), qMin(min, max), this->maximum());
    const int maxPosition =
        qBound(this->minimum(), qMax(min, max), this->maximum());

    bool emitMinPosChanged = (minPosition != d->m_MinimumPosition);
    bool emitMaxPosChanged = (maxPosition != d->m_MaximumPosition);

    if (!emitMinPosChanged && !emitMaxPosChanged) {
        return;
    }

    d->m_MinimumPosition = minPosition;
    d->m_MaximumPosition = maxPosition;

    if (!this->hasTracking()) {
        this->update();
    }
    if (isSliderDown()) {
        if (emitMinPosChanged) {
            emit minimumPositionChanged(d->m_MinimumPosition);
        }
        if (emitMaxPosChanged) {
            emit maximumPositionChanged(d->m_MaximumPosition);
        }
        if (emitMinPosChanged || emitMaxPosChanged) {
            emit positionsChanged(d->m_MinimumPosition, d->m_MaximumPosition);
        }
    }
    if (this->hasTracking()) {
        this->triggerAction(SliderMove);
        this->setValues(d->m_MinimumPosition, d->m_MaximumPosition);
    }
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setSymmetricMoves(bool symmetry) {
    Q_D(ctkRangeSlider);
    d->m_SymmetricMoves = symmetry;
}

// --------------------------------------------------------------------------
bool ctkRangeSlider::symmetricMoves()const {
    Q_D(const ctkRangeSlider);
    return d->m_SymmetricMoves;
}

// --------------------------------------------------------------------------
void ctkRangeSlider::onRangeChanged(int _minimum, int _maximum) {
    Q_UNUSED(_minimum);
    Q_UNUSED(_maximum);
    Q_D(ctkRangeSlider);
    this->setValues(d->m_MinimumValue, d->m_MaximumValue);
}

// --------------------------------------------------------------------------
// Render
void ctkRangeSlider::paintEvent(QPaintEvent *) {
    Q_D(ctkRangeSlider);
    QStyleOptionSlider option;
    this->initStyleOption(&option);

    QStylePainter painter(this);
    option.subControls = QStyle::SC_SliderGroove;
    // Move to minimum to not highlight the SliderGroove.
    // On mac style, drawing just the slider groove also draws the handles,
    // therefore we give a negative (outside of view) position.
    option.sliderValue = this->minimum() - this->maximum();
    option.sliderPosition = this->minimum() - this->maximum();
    painter.drawComplexControl(QStyle::CC_Slider, option);

    option.sliderPosition = d->m_MinimumPosition;
    const QRect lr = style()->subControlRect(QStyle::CC_Slider,
                     &option,
                     QStyle::SC_SliderHandle,
                     this);
    option.sliderPosition = d->m_MaximumPosition;

    const QRect ur = style()->subControlRect(QStyle::CC_Slider,
                     &option,
                     QStyle::SC_SliderHandle,
                     this);

    QRect sr = style()->subControlRect(QStyle::CC_Slider,
                                       &option,
                                       QStyle::SC_SliderGroove,
                                       this);
    QRect rangeBox;
    if (option.orientation == Qt::Horizontal) {
        rangeBox = QRect(
                       QPoint(qMin(lr.center().x(), ur.center().x()), sr.center().y() - 2),
                       QPoint(qMax(lr.center().x(), ur.center().x()), sr.center().y() + 1));
    }
    else {
        rangeBox = QRect(
                       QPoint(sr.center().x() - 2, qMin(lr.center().y(), ur.center().y())),
                       QPoint(sr.center().x() + 1, qMax(lr.center().y(), ur.center().y())));
    }

    // -----------------------------
    // Render the range
    //
    QRect groove = this->style()->subControlRect(QStyle::CC_Slider,
                   &option,
                   QStyle::SC_SliderGroove,
                   this);
    groove.adjust(0, 0, -1, 0);

    // Create default colors based on the transfer function.
    //
    QColor highlight = this->palette().color(QPalette::Normal, QPalette::Highlight);
    QLinearGradient gradient;
    if (option.orientation == Qt::Horizontal) {
        gradient = QLinearGradient(groove.center().x(), groove.top(),
                                   groove.center().x(), groove.bottom());
    }
    else {
        gradient = QLinearGradient(groove.left(), groove.center().y(),
                                   groove.right(), groove.center().y());
    }

    // TODO: Set this based on the supplied transfer function
    //QColor l = Qt::darkGray;
    //QColor u = Qt::black;

    gradient.setColorAt(0, highlight.darker(120));
    gradient.setColorAt(1, highlight.lighter(160));

    painter.setPen(QPen(highlight.darker(150), 0));
    painter.setBrush(gradient);
    painter.drawRect(rangeBox.intersected(groove));

    //  -----------------------------------
    // Render the sliders
    //
    if (this->isMinimumSliderDown()) {
        d->drawMaximumSlider(&painter);
        d->drawMinimumSlider(&painter);
    }
    else {
        d->drawMinimumSlider(&painter);
        d->drawMaximumSlider(&painter);
    }
}

// --------------------------------------------------------------------------
// Standard Qt UI events
void ctkRangeSlider::mousePressEvent(QMouseEvent *mouseEvent) {
    Q_D(ctkRangeSlider);
    if (minimum() == maximum() || (mouseEvent->buttons() ^ mouseEvent->button())) {
        mouseEvent->ignore();
        return;
    }
    int mepos = this->orientation() == Qt::Horizontal ?
                mouseEvent->pos().x() : mouseEvent->pos().y();

    QStyleOptionSlider option;
    this->initStyleOption(&option);

    QRect handleRect;
    ctkRangeSliderPrivate::Handle handle_ = d->handleAtPos(mouseEvent->pos(), handleRect);

    if (handle_ != ctkRangeSliderPrivate::NoHandle) {
        d->m_SubclassPosition = (handle_ == ctkRangeSliderPrivate::MinimumHandle)?
                                d->m_MinimumPosition : d->m_MaximumPosition;

        // save the position of the mouse inside the handle for later
        d->m_SubclassClickOffset = mepos - (this->orientation() == Qt::Horizontal ?
                                            handleRect.left() : handleRect.top());

        this->setSliderDown(true);

        if (d->m_SelectedHandles != handle_) {
            d->m_SelectedHandles = handle_;
            this->update(handleRect);
        }
        // Accept the mouseEvent
        mouseEvent->accept();
        return;
    }

    // if we are here, no handles have been pressed
    // Check if we pressed on the groove between the 2 handles

    QStyle::SubControl control = this->style()->hitTestComplexControl(
                                     QStyle::CC_Slider, &option, mouseEvent->pos(), this);
    QRect sr = style()->subControlRect(
                   QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this);
    int minCenter = (this->orientation() == Qt::Horizontal ?
                     handleRect.left() : handleRect.top());
    int maxCenter = (this->orientation() == Qt::Horizontal ?
                     handleRect.right() : handleRect.bottom());
    if (control == QStyle::SC_SliderGroove &&
            mepos > minCenter && mepos < maxCenter) {
        // warning lost of precision it might be fatal
        d->m_SubclassPosition = (d->m_MinimumPosition + d->m_MaximumPosition) / 2.;
        d->m_SubclassClickOffset = mepos - d->pixelPosFromRangeValue(d->m_SubclassPosition);
        d->m_SubclassWidth = (d->m_MaximumPosition - d->m_MinimumPosition) / 2;
        qMax(d->m_SubclassPosition - d->m_MinimumPosition, d->m_MaximumPosition - d->m_SubclassPosition);
        this->setSliderDown(true);
        if (!this->isMinimumSliderDown() || !this->isMaximumSliderDown()) {
            d->m_SelectedHandles =
                QFlags<ctkRangeSliderPrivate::Handle>(ctkRangeSliderPrivate::MinimumHandle) |
                QFlags<ctkRangeSliderPrivate::Handle>(ctkRangeSliderPrivate::MaximumHandle);
            this->update(handleRect.united(sr));
        }
        mouseEvent->accept();
        return;
    }
    mouseEvent->ignore();
}

// --------------------------------------------------------------------------
// Standard Qt UI events
void ctkRangeSlider::mouseMoveEvent(QMouseEvent *mouseEvent) {
    Q_D(ctkRangeSlider);
    if (!d->m_SelectedHandles) {
        mouseEvent->ignore();
        return;
    }
    int mepos = this->orientation() == Qt::Horizontal ?
                mouseEvent->pos().x() : mouseEvent->pos().y();

    QStyleOptionSlider option;
    this->initStyleOption(&option);

    const int m = style()->pixelMetric(QStyle::PM_MaximumDragDistance, &option, this);

    int newPosition = d->pixelPosToRangeValue(mepos - d->m_SubclassClickOffset);

    if (m >= 0) {
        const QRect r = rect().adjusted(-m, -m, m, m);
        if (!r.contains(mouseEvent->pos())) {
            newPosition = d->m_SubclassPosition;
        }
    }

    // Only the lower/left slider is down
    if (this->isMinimumSliderDown() && !this->isMaximumSliderDown()) {
        double newMinPos = qMin(newPosition,d->m_MaximumPosition);
        this->setPositions(newMinPos, d->m_MaximumPosition +
                           (d->m_SymmetricMoves ? d->m_MinimumPosition - newMinPos : 0));
    }
    // Only the upper/right slider is down
    else if (this->isMaximumSliderDown() && !this->isMinimumSliderDown()) {
        double newMaxPos = qMax(d->m_MinimumPosition, newPosition);
        this->setPositions(d->m_MinimumPosition -
                           (d->m_SymmetricMoves ? newMaxPos - d->m_MaximumPosition: 0),
                           newMaxPos);
    }
    // Both handles are down (the user clicked in between the handles)
    else if (this->isMinimumSliderDown() && this->isMaximumSliderDown()) {
        this->setPositions(newPosition - d->m_SubclassWidth,
                           newPosition + d->m_SubclassWidth);
    }
    mouseEvent->accept();
}

// --------------------------------------------------------------------------
// Standard Qt UI mouseEvents
void ctkRangeSlider::mouseReleaseEvent(QMouseEvent *mouseEvent) {
    Q_D(ctkRangeSlider);
    this->QSlider::mouseReleaseEvent(mouseEvent);

    setSliderDown(false);
    d->m_SelectedHandles = ctkRangeSliderPrivate::NoHandle;

    this->update();
}

// --------------------------------------------------------------------------
bool ctkRangeSlider::isMinimumSliderDown()const {
    Q_D(const ctkRangeSlider);
    return d->m_SelectedHandles & ctkRangeSliderPrivate::MinimumHandle;
}

// --------------------------------------------------------------------------
bool ctkRangeSlider::isMaximumSliderDown()const {
    Q_D(const ctkRangeSlider);
    return d->m_SelectedHandles & ctkRangeSliderPrivate::MaximumHandle;
}

// --------------------------------------------------------------------------
void ctkRangeSlider::initMinimumSliderStyleOption(QStyleOptionSlider *option) const {
    this->initStyleOption(option);
}

// --------------------------------------------------------------------------
void ctkRangeSlider::initMaximumSliderStyleOption(QStyleOptionSlider *option) const {
    this->initStyleOption(option);
}

// --------------------------------------------------------------------------
QString ctkRangeSlider::handleToolTip()const {
    Q_D(const ctkRangeSlider);
    return d->m_HandleToolTip;
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setHandleToolTip(const QString &_toolTip) {
    Q_D(ctkRangeSlider);
    d->m_HandleToolTip = _toolTip;
}

// --------------------------------------------------------------------------
bool ctkRangeSlider::event(QEvent *_event) {
    Q_D(ctkRangeSlider);
    switch (_event->type()) {
    case QEvent::ToolTip: {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(_event);
        QStyleOptionSlider opt;
        // Test the MinimumHandle
        opt.sliderPosition = d->m_MinimumPosition;
        opt.sliderValue = d->m_MinimumValue;
        this->initStyleOption(&opt);
        QStyle::SubControl hoveredControl =
            this->style()->hitTestComplexControl(
                QStyle::CC_Slider, &opt, helpEvent->pos(), this);
        if (!d->m_HandleToolTip.isEmpty() &&
                hoveredControl == QStyle::SC_SliderHandle) {
            QToolTip::showText(helpEvent->globalPos(), d->m_HandleToolTip.arg(this->minimumValue()));
            _event->accept();
            return true;
        }
        // Test the MaximumHandle
        opt.sliderPosition = d->m_MaximumPosition;
        opt.sliderValue = d->m_MaximumValue;
        this->initStyleOption(&opt);
        hoveredControl = this->style()->hitTestComplexControl(
                             QStyle::CC_Slider, &opt, helpEvent->pos(), this);
        if (!d->m_HandleToolTip.isEmpty() &&
                hoveredControl == QStyle::SC_SliderHandle) {
            QToolTip::showText(helpEvent->globalPos(), d->m_HandleToolTip.arg(this->maximumValue()));
            _event->accept();
            return true;
        }
    }
    default:
        break;
    }
    return this->Superclass::event(_event);
}




trim::trim(QWidget *parent, QVector<QString> dicom_struct_name, QVector<QString> dicom_struct_type, QVector<double> xbound, QVector<double> ybound, QVector<double> zbound, QVector<QVector<QVector<double>>> extrema, QVector <double> seed_bounds)
    : QWidget(parent) { // Pass the parameter to the QWidget constructor
    mom = parent;

    x = xbound;
    y = ybound;
    z = zbound;
    contourBounds = extrema;
    seedBounds = seed_bounds;

    mediaItems = new QStringList();
    for (int i=0; i<dicom_struct_name.size(); i++) {
        *mediaItems<<QString(dicom_struct_name[i] + " (" + dicom_struct_type[i].toLower() + ")");
    }


    createLayout(); // This creates the layout structure
    connectLayout(); // This connects the layout with the SLOTs

}

//Overloaded function - this is used for the middle tab - and all functions with Mid are used for the middle tab
trim::trim(QWidget *parent, QVector<double> xbound, QVector<double> ybound, QVector<double> zbound)
    : QWidget(parent) { // Pass the parameter to the QWidget constructor
    mom = parent;

    x = xbound;
    y = ybound;
    z = zbound;


    createLayoutMid(); // This creates the layout structure
    connectLayoutMid(); // This connects the layout with the SLOTs

}

trim::~trim() {


}




void trim::createLayout() {

    indexChange = 0;
    window = new QWidget();
    QGridLayout *trim_layout = new QGridLayout();

    trimType = new QComboBox();
    QStringList trimList = {"Trim bounds using contour boundaries", "Trim bounds using seed positions", "Trim bounds manually"};
    trimType->addItems(trimList);

    contourBox = new QGroupBox(tr("Trim the egsphant"));
    contourBox->setCheckable(true);
    contourBox->setChecked(false);
    mediaBox = new QComboBox();
    mediaBox->addItems(*mediaItems);
    mediaBox->setCurrentIndex(0);

    mediaLabel = new QLabel(tr("Contour Selected:"));
    QGridLayout *mediaLayout = new QGridLayout();
    mediaLayout->addWidget(mediaLabel,0,0);
    mediaLayout->addWidget(mediaBox,0,1,1,2);

    xLabel = new QLabel(tr("X-axis boundaries"));
    xLabel->setToolTip(tr("Adjust the slider to select the appropriate x-axis boundaries \n") +
                       tr("Slider is positioned using the contour exterma"));
    xSlider = new ctkRangeSlider(Qt::Horizontal);
    xSlider->setRange(0, x.size()-1);
    xSlider->setPositions(get_idx_from_bounds(contourBounds[0][0][0],x), get_idx_from_bounds(contourBounds[0][0][1],x));
    xMin = new QLabel();
    xMin->setText(QString::number(contourBounds[0][0][0]));
    xMax = new QLabel();
    xMax->setText(QString::number(contourBounds[0][0][1]));

    yLabel = new QLabel(tr("Y-axis boundaries"));
    ySlider = new ctkRangeSlider(Qt::Horizontal);
    yLabel->setToolTip(tr("Adjust the slider to select the appropriate y-axis boundaries \n") +
                       tr("Slider is positioned using the contour exterma"));
    ySlider->setRange(0, y.size()-1);
    ySlider->setPositions(get_idx_from_bounds(contourBounds[0][1][0],y), get_idx_from_bounds(contourBounds[0][1][1],y));
    yMin = new QLabel();
    yMin->setText(QString::number(contourBounds[0][1][0]));
    yMax = new QLabel();
    yMax->setText(QString::number(contourBounds[0][1][1]));

    zLabel = new QLabel(tr("Z-axis boundaries"));
    zLabel->setToolTip(tr("Adjust the slider to select the appropriate z-axis boundaries \n") +
                       tr("Slider is positioned using the contour exterma"));
    zSlider = new ctkRangeSlider(Qt::Horizontal);
    zSlider->setRange(0,z.size()-1);
    zSlider->setPositions(get_idx_from_bounds(contourBounds[0][2][0],z), get_idx_from_bounds(contourBounds[0][2][1],z));
    zMin = new QLabel(QString::number(contourBounds[0][2][0]));
    zMax = new QLabel(QString::number(contourBounds[0][2][1]));

    QGridLayout *xGrid = new QGridLayout();
    xGrid->addWidget(xLabel,1,0,1,1);
    xGrid->addWidget(xSlider,2,1,1,3);
    xGrid->addWidget(xMin,3,1);
    xGrid->addWidget(xMax,3,3);



    QGridLayout *yGrid = new QGridLayout();
    yGrid->addWidget(yLabel,1,0,1,1);
    yGrid->addWidget(ySlider,2,1,1,3);
    yGrid->addWidget(yMin,3,1);
    yGrid->addWidget(yMax,3,3);

    QGridLayout *zGrid = new QGridLayout();
    zGrid->addWidget(zLabel,1,0,1,1);
    zGrid->addWidget(zSlider,2,1,1,3);
    zGrid->addWidget(zMin,3,1);
    zGrid->addWidget(zMax,3,3);

    note = new QLabel();

    QGridLayout *boundsLayout = new QGridLayout();
    boundsLayout->addWidget(trimType, 0, 0, 1, 3);
    boundsLayout->addLayout(mediaLayout, 1, 0, 1, 3);
    boundsLayout->addLayout(xGrid,2,0,2,3);
    boundsLayout->addLayout(yGrid,4,0,2,3);
    boundsLayout->addLayout(zGrid,6,0,2,3);
    boundsLayout->addWidget(note, 8, 0, 1, 3);
    contourBox->setLayout(boundsLayout);

    groupBoxP = new QGroupBox(tr("Inscribe the patient phantom within a geometry"));
    groupBoxP->setCheckable(true);
    groupBoxP->setChecked(false);
    boxPhantPath = new QLabel();
    boxPhantPath->setToolTip(tr("The path of the selected geometry."));
    select1 = new QPushButton(tr("Select the geometry file"));
    phantHBox = new QHBoxLayout();
    phantHBox->addWidget(boxPhantPath);
    phantHBox->addWidget(select1);
    groupBoxP->setLayout(phantHBox);
    groupBoxP->setToolTip(tr("The selected geometry will be used to hold the entire phantom."));

    groupBoxA = new QGroupBox(tr("Insert an applicator within the phantom"));
    groupBoxA->setCheckable(true);
    groupBoxA->setChecked(false);
    applicPhantPath = new QLabel();
    applicPhantPath->setToolTip(tr("The path of the selected applicator."));
    select2 = new QPushButton(tr("Select the applicator file"));
    applicHBox = new QHBoxLayout();
    applicHBox->addWidget(applicPhantPath);
    applicHBox->addWidget(select2);
    groupBoxA->setLayout(applicHBox);
    groupBoxP->setToolTip(tr("The brachytherapy seeds will be inscribed within the selected geometry."));

    cancel = new QPushButton("Cancel");
    apply = new QPushButton("Apply");
    QGridLayout *ExitLayout = new QGridLayout();
    ExitLayout->addWidget(cancel, 0, 0); //, 1, 2); //FIX this spacing
    ExitLayout->addWidget(apply, 0, 2); //, 3, 4);
    QGroupBox *ExitFrame = new QGroupBox();
    ExitFrame->setLayout(ExitLayout);


    trim_layout->addWidget(contourBox, 0, 0, 1, 3);
    trim_layout->addWidget(groupBoxP,1,0,1,3);
    trim_layout->addWidget(groupBoxA,2,0,1,3);
    trim_layout->addWidget(ExitFrame,  3, 2, 1, 1);

    if (mediaItems->size() == 0) { //there are no contours in the diocm data
        indexChange--;
        trimType->removeItem(0);

        if (seedBounds.size() == 0) { //there are also no seeds
            indexChange--;
            trimType->removeItem(0);
        }
        toggleTrimBox(0);
    }
    else if (seedBounds.size() == 0) { //there are no seeds but there is contour data
        indexChange--;
        trimType->removeItem(1);
        toggleTrimBox(0);
    }

    window-> setLayout(trim_layout); //this may be writing over the 'main' window
    window->setWindowTitle(tr("Trim the phantom"));
    window->setHidden(TRUE);

    save_options_on_startup("");
}



void trim::connectLayout() {

    connect(mediaBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changed_contour(int)));
    connect(trimType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(toggleTrimBox(int)));

    connect(zSlider, SIGNAL(minimumPositionChanged(int)),
            this, SLOT(changed_zmin_slider(int)));
    connect(zSlider, SIGNAL(maximumPositionChanged(int)),
            this, SLOT(changed_zmax_slider(int)));

    connect(xSlider, SIGNAL(minimumPositionChanged(int)),
            this, SLOT(changed_xmin_slider(int)));
    connect(xSlider, SIGNAL(maximumPositionChanged(int)),
            this, SLOT(changed_xmax_slider(int)));

    connect(ySlider, SIGNAL(minimumPositionChanged(int)),
            this, SLOT(changed_ymin_slider(int)));
    connect(ySlider, SIGNAL(maximumPositionChanged(int)),
            this, SLOT(changed_ymax_slider(int)));


    connect(select1, SIGNAL(clicked()),
            this, SLOT(select_phant_toInscribe()));
    connect(select2, SIGNAL(clicked()),
            this, SLOT(select_applicator()));



    connect(cancel, SIGNAL(clicked()),
            this, SLOT(close_options()));
    connect(apply, SIGNAL(clicked()),
            this, SLOT(set_values()));

}


void trim::createLayoutMid() {
    window = new QWidget();
    QGridLayout *trim_layout = new QGridLayout();

    contourBox = new QGroupBox(tr("Trim the egsphant"));
    contourBox->setCheckable(true);
    contourBox->setChecked(false);

    xLabel = new QLabel(tr("X-axis boundaries"));
    xLabel->setToolTip(tr("Adjust the slider to select the appropriate x-axis boundaries \n") +
                       tr("Slider is positioned using the contour exterma"));
    xSlider = new ctkRangeSlider(Qt::Horizontal);
    xSlider->setRange(0, x.size()-1);
    xSlider->setPositions(get_idx_from_bounds(x[0],x), get_idx_from_bounds(x[x.size()-1],x));
    xMin = new QLabel();
    xMin->setText(QString::number(x[0]));
    xMax = new QLabel();
    xMax->setText(QString::number(x[x.size()-1]));

    yLabel = new QLabel(tr("Y-axis boundaries"));
    ySlider = new ctkRangeSlider(Qt::Horizontal);
    yLabel->setToolTip(tr("Adjust the slider to select the appropriate y-axis boundaries \n") +
                       tr("Slider is positioned using the contour exterma"));
    ySlider->setRange(0, y.size()-1);
    ySlider->setPositions(get_idx_from_bounds(y[0],y), get_idx_from_bounds(y[y.size()-1],y));
    yMin = new QLabel();
    yMin->setText(QString::number(y[0]));
    yMax = new QLabel();
    yMax->setText(QString::number(y[y.size()-1]));

    zLabel = new QLabel(tr("Z-axis boundaries"));
    zLabel->setToolTip(tr("Adjust the slider to select the appropriate z-axis boundaries \n") +
                       tr("Slider is positioned using the contour exterma"));
    zSlider = new ctkRangeSlider(Qt::Horizontal);
    zSlider->setRange(0,z.size()-1);
    zSlider->setPositions(get_idx_from_bounds(z[0],z), get_idx_from_bounds(z[z.size()-1],z));
    zMin = new QLabel(QString::number(z[0]));
    zMax = new QLabel(QString::number(z[z.size()-1]));

    QGridLayout *xGrid = new QGridLayout();
    xGrid->addWidget(xLabel,1,0,1,1);
    xGrid->addWidget(xSlider,2,1,1,3);
    xGrid->addWidget(xMin,3,1);
    xGrid->addWidget(xMax,3,3);

    QGridLayout *yGrid = new QGridLayout();
    yGrid->addWidget(yLabel,1,0,1,1);
    yGrid->addWidget(ySlider,2,1,1,3);
    yGrid->addWidget(yMin,3,1);
    yGrid->addWidget(yMax,3,3);

    QGridLayout *zGrid = new QGridLayout();
    zGrid->addWidget(zLabel,1,0,1,1);
    zGrid->addWidget(zSlider,2,1,1,3);
    zGrid->addWidget(zMin,3,1);
    zGrid->addWidget(zMax,3,3);

    QGridLayout *boundsLayout = new QGridLayout();
    boundsLayout->addLayout(xGrid,0,0,2,3);
    boundsLayout->addLayout(yGrid,2,0,2,3);
    boundsLayout->addLayout(zGrid,4,0,2,3);
    contourBox->setLayout(boundsLayout);

    if (x.size() == 3 && y.size() == 3 && z.size() == 3)
        if (x[0] == 0 && x[1] == 0 && x[2] == 0 &&
                y[0] == 0 && y[1] == 0 && y[2] == 0 &&
                z[0] == 0 && z[1] == 0 && z[2] == 0) {
            contourBox->setDisabled(true);
            contourBox->setToolTip(tr("To enable this pane, an .egsphant file must be selected."));
        }

    groupBoxP = new QGroupBox(tr("Inscribe the patient phantom within a geometry"));
    groupBoxP->setCheckable(true);
    groupBoxP->setChecked(false);
    boxPhantPath = new QLabel();
    boxPhantPath->setToolTip(tr("The path of the selected geometry."));
    select1 = new QPushButton(tr("Select the geometry file"));
    phantHBox = new QHBoxLayout();
    phantHBox->addWidget(boxPhantPath);
    phantHBox->addWidget(select1);
    groupBoxP->setLayout(phantHBox);
    groupBoxP->setToolTip(tr("The selected geometry will be used to hold the entire phantom."));

    groupBoxA = new QGroupBox(tr("Insert an applicator within the phantom"));
    groupBoxA->setCheckable(true);
    groupBoxA->setChecked(false);
    applicPhantPath = new QLabel();
    applicPhantPath->setToolTip(tr("The path of the selected applicator."));
    select2 = new QPushButton(tr("Select the applicator file"));
    applicHBox = new QHBoxLayout();
    applicHBox->addWidget(applicPhantPath);
    applicHBox->addWidget(select2);
    groupBoxA->setLayout(applicHBox);
    groupBoxP->setToolTip(tr("The brachytherapy seeds will be inscribed within the selected geometry."));

    cancel = new QPushButton("Cancel");
    apply = new QPushButton("Apply");
    QGridLayout *ExitLayout = new QGridLayout();
    ExitLayout->addWidget(cancel, 0, 0); //, 1, 2); //FIX this spacing
    ExitLayout->addWidget(apply, 0, 2); //, 3, 4);
    QGroupBox *ExitFrame = new QGroupBox();
    ExitFrame->setLayout(ExitLayout);

    trim_layout->addWidget(contourBox, 0, 0, 1, 3);
    trim_layout->addWidget(groupBoxP,1,0,1,3);
    trim_layout->addWidget(groupBoxA,2,0,1,3);
    trim_layout->addWidget(ExitFrame,  3, 2, 1, 1);


    window-> setLayout(trim_layout); //this may be writing over the 'main' window
    window->setWindowTitle(tr("Trim the phantom"));
    window->setHidden(TRUE);

    save_options_on_startupMid("");

}



void trim::connectLayoutMid() {
    connect(zSlider, SIGNAL(minimumPositionChanged(int)),
            this, SLOT(changed_zmin_slider(int)));
    connect(zSlider, SIGNAL(maximumPositionChanged(int)),
            this, SLOT(changed_zmax_slider(int)));

    connect(xSlider, SIGNAL(minimumPositionChanged(int)),
            this, SLOT(changed_xmin_slider(int)));
    connect(xSlider, SIGNAL(maximumPositionChanged(int)),
            this, SLOT(changed_xmax_slider(int)));

    connect(ySlider, SIGNAL(minimumPositionChanged(int)),
            this, SLOT(changed_ymin_slider(int)));
    connect(ySlider, SIGNAL(maximumPositionChanged(int)),
            this, SLOT(changed_ymax_slider(int)));


    connect(select1, SIGNAL(clicked()),
            this, SLOT(select_phant_toInscribe()));
    connect(select2, SIGNAL(clicked()),
            this, SLOT(select_applicator()));


    connect(cancel, SIGNAL(clicked()),
            this, SLOT(close_optionsMid()));
    connect(apply, SIGNAL(clicked()),
            this, SLOT(set_valuesMid()));

}

void trim::reset_bounds() {
    xSlider->setRange(0, x.size()-1);
    xSlider->setPositions(0, x.size()-1);
    xMin->setText(QString::number(x[0]));
    xMax->setText(QString::number(x[x.size()-1]));
    ySlider->setRange(0, y.size()-1);
    ySlider->setPositions(0, y.size()-1);
    yMin->setText(QString::number(y[0]));
    yMax->setText(QString::number(y[y.size()-1]));
    zSlider->setRange(0, z.size()-1);
    zSlider->setPositions(0, z.size()-1);
    zMin->setText(QString::number(z[0]));
    zMax->setText(QString::number(z[z.size()-1]));
    note->setText(tr("Note: the egsphant was previously trimmed. The new boundaries reflect the changes."));
}




void trim::toggleTrimBox(int index) {

    if (index + indexChange == 0) { //Contour boundaries
        mediaBox->setEnabled(true);
        mediaLabel->setEnabled(true);

        changed_contour(index);

        xLabel->setToolTip(tr("Adjust the slider to select the appropriate x-axis boundaries \n") +
                           tr("Slider is positioned using the contour exterma"));
        yLabel->setToolTip(tr("Adjust the slider to select the appropriate y-axis boundaries \n") +
                           tr("Slider is positioned using the contour exterma"));
        zLabel->setToolTip(tr("Adjust the slider to select the appropriate z-axis boundaries \n") +
                           tr("Slider is positioned using the contour exterma"));

    }
    else if (index  + indexChange == 1) {  //Seed boundaries
        mediaBox->setDisabled(true);
        mediaLabel->setDisabled(true);

        if (seedBounds[0] < x[0]) {
            xSlider->setMinimumPosition(0);
            xMin->setText(QString::number(x[0]));
        }
        else {
            xSlider->setMinimumPosition(get_idx_from_bounds(seedBounds[0],x));
            xMin->setText(QString::number(seedBounds[0]));
        }

        if (seedBounds[1] > x[x.size()-1]) {
            xSlider->setMaximumPosition(x.size()-1);
            xMax->setText(QString::number(x[x.size()-1]));
        }
        else {
            xSlider->setMaximumPosition(get_idx_from_bounds(seedBounds[1],x));
            xMax->setText(QString::number(seedBounds[1]));
        }

        if (seedBounds[2] < y[0]) {
            ySlider->setMinimumPosition(0);
            yMin->setText(QString::number(y[0]));
        }
        else {
            ySlider->setMinimumPosition(get_idx_from_bounds(seedBounds[2],y));
            yMin->setText(QString::number(seedBounds[2]));
        }

        if (seedBounds[3] > y[y.size()-1]) {
            ySlider->setMaximumPosition(y.size()-1);
            yMax->setText(QString::number(y[y.size()-1]));
        }
        else {
            ySlider->setMaximumPosition(get_idx_from_bounds(seedBounds[3],y));
            yMax->setText(QString::number(seedBounds[3]));
        }

        if (seedBounds[4] < z[0]) {
            zSlider->setMinimumPosition(0);
            zMin->setText(QString::number(z[0]));
        }
        else {
            zSlider->setMinimumPosition(get_idx_from_bounds(seedBounds[4],z));
            zMin->setText(QString::number(seedBounds[4]));
        }

        if (seedBounds[5] > z[z.size()-1]) {
            zSlider->setMaximumPosition(z.size()-1);
            zMax->setText(QString::number(z[z.size()-1]));
        }
        else {
            zSlider->setMaximumPosition(get_idx_from_bounds(seedBounds[5],z));
            zMax->setText(QString::number(seedBounds[5]));
        }

        xLabel->setToolTip(tr("Adjust the slider to select the appropriate x-axis boundaries \n") +
                           tr("Slider is positioned using the source position exterma"));
        yLabel->setToolTip(tr("Adjust the slider to select the appropriate y-axis boundaries \n") +
                           tr("Slider is positioned using the source position exterma"));
        zLabel->setToolTip(tr("Adjust the slider to select the appropriate z-axis boundaries \n") +
                           tr("Slider is positioned using the source position exterma"));


    }
    else if (index  + indexChange == 2) {  //Manual boundaries
        mediaBox->setDisabled(true);
        mediaLabel->setDisabled(true);

        xSlider->setPositions(0, x.size()-1);
        xMin->setText(QString::number(x[0]));
        xMax->setText(QString::number(x[x.size()-1]));

        ySlider->setPositions(0, y.size()-1);
        yMin->setText(QString::number(y[0]));
        yMax->setText(QString::number(y[y.size()-1]));

        zSlider->setPositions(0, z.size()-1);
        zMin->setText(QString::number(z[0]));
        zMax->setText(QString::number(z[z.size()-1]));

        xLabel->setToolTip(tr("Adjust the slider to select the appropriate x-axis boundaries \n"));
        yLabel->setToolTip(tr("Adjust the slider to select the appropriate y-axis boundaries \n"));
        zLabel->setToolTip(tr("Adjust the slider to select the appropriate z-axis boundaries \n"));

    }
}



void trim::changed_contour(int index) {
    // xSlider->setPositions(get_idx_from_bounds(contourBounds[index][0][0],x), get_idx_from_bounds(contourBounds[index][0][1],x));
    // xMin->setText(QString::number(contourBounds[index][0][0]));
    // xMax->setText(QString::number(contourBounds[index][0][1]));

    // ySlider->setPositions(get_idx_from_bounds(contourBounds[index][1][0],y), get_idx_from_bounds(contourBounds[index][1][1],y));
    // yMin->setText(QString::number(contourBounds[index][1][0]));
    // yMax->setText(QString::number(contourBounds[index][1][1]));

    // zSlider->setPositions(get_idx_from_bounds(contourBounds[index][2][0],z), get_idx_from_bounds(contourBounds[index][2][1],z));
    // zMin->setText(QString::number(contourBounds[index][2][0]));
    // zMax->setText(QString::number(contourBounds[index][2][1]));
    if (contourBounds[index][0][0] < x[0]) {
        xSlider->setMinimumPosition(0);
        xMin->setText(QString::number(x[0]));
    }
    else {
        xSlider->setMinimumPosition(get_idx_from_bounds(contourBounds[index][0][0],x));
        xMin->setText(QString::number(contourBounds[index][0][0]));
    }

    if (contourBounds[index][0][1] > x[x.size()-1]) {
        xSlider->setMaximumPosition(x.size()-1);
        xMax->setText(QString::number(x[x.size()-1]));
    }
    else {
        xSlider->setMaximumPosition(get_idx_from_bounds(contourBounds[index][0][1],x));
        xMax->setText(QString::number(contourBounds[index][0][1]));
    }

    if (contourBounds[index][1][0] < y[0]) {
        ySlider->setMinimumPosition(0);
        yMin->setText(QString::number(y[0]));
    }
    else {
        ySlider->setMinimumPosition(get_idx_from_bounds(contourBounds[index][1][0],y));
        yMin->setText(QString::number(contourBounds[index][1][0]));
    }

    if (contourBounds[index][1][1] > y[y.size()-1]) {
        ySlider->setMaximumPosition(y.size()-1);
        yMax->setText(QString::number(y[y.size()-1]));
    }
    else {
        ySlider->setMaximumPosition(get_idx_from_bounds(contourBounds[index][1][1],y));
        yMax->setText(QString::number(contourBounds[index][1][1]));
    }

    if (contourBounds[index][2][0] < z[0]) {
        zSlider->setMinimumPosition(0);
        zMin->setText(QString::number(z[0]));
    }
    else {
        zSlider->setMinimumPosition(get_idx_from_bounds(contourBounds[index][2][0],z));
        zMin->setText(QString::number(contourBounds[index][2][0]));
    }

    if (contourBounds[index][2][1] > z[z.size()-1]) {
        zSlider->setMaximumPosition(z.size()-1);
        zMax->setText(QString::number(z[z.size()-1]));
    }
    else {
        zSlider->setMaximumPosition(get_idx_from_bounds(contourBounds[index][2][1],z));
        zMax->setText(QString::number(contourBounds[index][2][1]));
    }

}


void trim::changed_zmin_slider(int val) {
	(void)val; // Remove unused parameter warning
    zMin->setText(QString::number(z[zSlider->minimumPosition()]));
}

void trim::changed_zmax_slider(int val) {
	(void)val; // Remove unused parameter warning
    zMax->setText(QString::number(z[zSlider->maximumPosition()]));
}

void trim::changed_xmin_slider(int val) {
	(void)val; // Remove unused parameter warning
    xMin->setText(QString::number(x[xSlider->minimumPosition()]));
}

void trim::changed_xmax_slider(int val) {
	(void)val; // Remove unused parameter warning
    xMax->setText(QString::number(x[xSlider->maximumPosition()]));
}

void trim::changed_ymin_slider(int val) {
	(void)val; // Remove unused parameter warning
    yMin->setText(QString::number(y[ySlider->minimumPosition()]));
}

void trim::changed_ymax_slider(int val) {
	(void)val; // Remove unused parameter warning
    yMax->setText(QString::number(y[ySlider->maximumPosition()]));
}


int trim::get_idx_from_bounds(double pos, QVector<double> bounds)
// get_idx_from_bounds returns the 1 dimensional index of the voxel
// whose bounds contain pos
{
    if ((pos < bounds[0])) {
        return 0;
    }
    if (bounds.back() < pos) {
        return bounds.size() - 1;
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

/***
Function: close_options
-----------------------
The fields are returned to their original values when the user clicked the 'cancel' button

***/
void trim::close_options() {
    this->setDisabled(TRUE);
    mom->setEnabled(TRUE);
    window->hide();

    //Reset data
    resetTrimOptions();
    boxPhantPath->setText(phantPath);
    applicPhantPath->setText(applicPath);

    //Reset the checked/notchecked screens
    if (phantChecked) {
        groupBoxP->setChecked(true);
    }
    else {
        groupBoxP->setChecked(false);
    }


    if (applicChecked) {
        groupBoxA->setChecked(true);
    }
    else {
        groupBoxA->setChecked(false);
    }


    if (trimChecked) {
        contourBox->setChecked(true);
    }
    else {
        contourBox->setChecked(false);
    }

    emit(closed());
}


void trim::close_optionsMid() {
    this->setDisabled(TRUE);
    mom->setEnabled(TRUE);
    window->hide();

    //Reset data
    resetTrimOptionsMid();
    boxPhantPath->setText(phantPath);
    applicPhantPath->setText(applicPath);

    //Reset the checked/notchecked screens
    if (phantChecked) {
        groupBoxP->setChecked(true);
    }
    else {
        groupBoxP->setChecked(false);
    }


    if (applicChecked) {
        groupBoxA->setChecked(true);
    }
    else {
        groupBoxA->setChecked(false);
    }


    if (trimChecked) {
        contourBox->setChecked(true);
    }
    else {
        contourBox->setChecked(false);
    }

    emit(closed());
}

//reset the trim options to what they were on opening the window
void trim::resetTrimOptions() {
    //Reset options
    trimType->setCurrentIndex(trimTypeIndex);

    if (trimTypeIndex == 0) { //save the contour index
        mediaBox->setCurrentIndex(contourIndex);
    }

    //Save the selected slider/bounds
    xMin->setText(QString::number(xMinVal));
    xMax->setText(QString::number(xMaxVal));
    yMin->setText(QString::number(yMinVal));
    yMax->setText(QString::number(yMaxVal));
    zMin->setText(QString::number(zMinVal));
    zMax->setText(QString::number(zMaxVal));

    xMinIndex = get_idx_from_bounds(xMin->text().toDouble(),x);
    xMaxIndex = get_idx_from_bounds(xMax->text().toDouble(),x);
    yMinIndex = get_idx_from_bounds(yMin->text().toDouble(),y);
    yMaxIndex = get_idx_from_bounds(yMax->text().toDouble(),y);
    zMinIndex = get_idx_from_bounds(zMin->text().toDouble(),z);
    zMaxIndex = get_idx_from_bounds(zMax->text().toDouble(),z);

}

void trim::resetTrimOptionsMid() {
    //Save the selected slider/bounds
    xMin->setText(QString::number(xMinVal));
    xMax->setText(QString::number(xMaxVal));
    yMin->setText(QString::number(yMinVal));
    yMax->setText(QString::number(yMaxVal));
    zMin->setText(QString::number(zMinVal));
    zMax->setText(QString::number(zMaxVal));

    xMinIndex = get_idx_from_bounds(xMin->text().toDouble(),x);
    xMaxIndex = get_idx_from_bounds(xMax->text().toDouble(),x);
    yMinIndex = get_idx_from_bounds(yMin->text().toDouble(),y);
    yMaxIndex = get_idx_from_bounds(yMax->text().toDouble(),y);
    zMinIndex = get_idx_from_bounds(zMin->text().toDouble(),z);
    zMaxIndex = get_idx_from_bounds(zMax->text().toDouble(),z);

    xSlider->setPositions(xMinIndex, xMaxIndex);
    ySlider->setPositions(yMinIndex, yMaxIndex);
    zSlider->setPositions(zMinIndex, zMaxIndex);
}



/***
Function: save_options_on_startup
--------------------------------
Process: This functions saves the parameters when the window is opened
         Parameters are used if cancel is pressed
***/
void trim::save_options_on_startup(QString egsinp) {
	(void)egsinp; // Remove unused parameter warning
    if (groupBoxP->isChecked()) {
        phantChecked = true;
    }
    else {
        phantChecked = false;
    }

    if (groupBoxA->isChecked()) {
        applicChecked = true;
    }
    else {
        applicChecked = false;
    }

    if (contourBox->isChecked()) {
        trimChecked = true;
    }
    else {
        trimChecked = false;
    }

    trimTypeIndex = trimType->currentIndex();

    if (trimTypeIndex == 0) { //save the contour index
        contourIndex = mediaBox->currentIndex();
    }

    //Save the selected slider/bounds
    xMinVal = xMin->text().toDouble();
    xMaxVal = xMax->text().toDouble();
    yMinVal = yMin->text().toDouble();
    yMaxVal = yMax->text().toDouble();
    zMinVal = zMin->text().toDouble();
    zMaxVal = zMax->text().toDouble();

    xMinIndex = get_idx_from_bounds(xMin->text().toDouble(),x);
    xMaxIndex = get_idx_from_bounds(xMax->text().toDouble(),x);
    yMinIndex = get_idx_from_bounds(yMin->text().toDouble(),y);
    yMaxIndex = get_idx_from_bounds(yMax->text().toDouble(),y);
    zMinIndex = get_idx_from_bounds(zMinVal,z);
    zMaxIndex = get_idx_from_bounds(zMaxVal,z);

    phantPath = boxPhantPath->text();
    applicPath = applicPhantPath->text();

    if (generatedPhant) {
        groupBoxP->setDisabled(true);
        groupBoxP->setToolTip(tr("The egsinp file has been created. The inscribed geometry cannot be changed."));
        groupBoxA->setDisabled(true);
        groupBoxA->setToolTip(tr("The egsinp file has been created. The applicator geometry cannot be changed."));
    }
}

/***
Function: save_options_on_startup
--------------------------------
Process: This functions saves the parameters when the window is opened
         Parameters are used if cancel is pressed
***/
void trim::save_options_on_startupMid(QString egsinp) {
	(void)egsinp; // Remove unused parameter warning
    if (groupBoxP->isChecked()) {
        phantChecked = true;
    }
    else {
        phantChecked = false;
    }

    if (groupBoxA->isChecked()) {
        applicChecked = true;
    }
    else {
        applicChecked = false;
    }

    if (contourBox->isChecked()) {
        trimChecked = true;
    }
    else {
        trimChecked = false;
    }


    //Save the selected slider/bounds
    xMinVal = xMin->text().toDouble();
    xMaxVal = xMax->text().toDouble();
    yMinVal = yMin->text().toDouble();
    yMaxVal = yMax->text().toDouble();
    zMinVal = zMin->text().toDouble();
    zMaxVal = zMax->text().toDouble();

    xMinIndex = get_idx_from_bounds(xMin->text().toDouble(),x);
    xMaxIndex = get_idx_from_bounds(xMax->text().toDouble(),x);
    yMinIndex = get_idx_from_bounds(yMin->text().toDouble(),y);
    yMaxIndex = get_idx_from_bounds(yMax->text().toDouble(),y);
    zMinIndex = get_idx_from_bounds(zMinVal,z);
    zMaxIndex = get_idx_from_bounds(zMaxVal,z);

    phantPath = boxPhantPath->text();
    applicPath = applicPhantPath->text();
}

/***
Functon: set_values
-------------------
Used to save the user changes

***/
void trim::set_values() {
    //Made changes to the trim options
    if (contourBox->isChecked()) {
        //Save the data
        xMinIndex = get_idx_from_bounds(xMin->text().toDouble(),x);
        xMaxIndex = get_idx_from_bounds(xMax->text().toDouble(),x);
        yMinIndex = get_idx_from_bounds(yMin->text().toDouble(),y);
        yMaxIndex = get_idx_from_bounds(yMax->text().toDouble(),y);
        zMinIndex = get_idx_from_bounds(zMin->text().toDouble(),z);
        zMaxIndex = get_idx_from_bounds(zMax->text().toDouble(),z);

        //If egsinp already exists...replace it and output new egsphant
        if (generatedPhant) { //egsphant has already been produced
            emit(trimExisting());
        }
        else {
            emit(trimNotExisting());
        }
    }
    else {
        //reset the trim options
        resetTrimOptions();
    }

    //Added the phant
    if (groupBoxP->isChecked()) {
        inscribedPhantPath = boxPhantPath->text();
    }
    else {
        inscribedPhantPath = "";
    }

    if (groupBoxA->isChecked()) {
        applicatorPath = applicPhantPath->text();
    }
    else {
        applicatorPath = "";
    }


    this->setDisabled(TRUE);
    mom->setEnabled(TRUE);
    window->hide();


}

void trim::set_valuesMid() {
    //Made changes to the trim options
    if (contourBox->isChecked()) {
        //Save the data
        xMinIndex = get_idx_from_bounds(xMin->text().toDouble(),x);
        xMaxIndex = get_idx_from_bounds(xMax->text().toDouble(),x);
        yMinIndex = get_idx_from_bounds(yMin->text().toDouble(),y);
        yMaxIndex = get_idx_from_bounds(yMax->text().toDouble(),y);
        zMinIndex = get_idx_from_bounds(zMin->text().toDouble(),z);
        zMaxIndex = get_idx_from_bounds(zMax->text().toDouble(),z);

    }
    else {
        //reset the trim options
        resetTrimOptionsMid();
    }

    //Added the phant
    if (groupBoxP->isChecked()) {
        inscribedPhantPath = boxPhantPath->text();
    }
    else {
        inscribedPhantPath = "";
    }

    if (groupBoxA->isChecked()) {
        applicatorPath = applicPhantPath->text();
    }
    else {
        applicatorPath = "";
    }


    this->setDisabled(TRUE);
    mom->setEnabled(TRUE);
    window->hide();

}



void trim::select_phant_toInscribe() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select the phant file"),
                       egs_path+ "/lib/geometry/phantoms/");
    if (!fileName.isEmpty()) {
        boxPhantPath->setText(fileName);
    }

}

void trim::select_applicator() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select the applicator file"),
                       egs_path + "/lib/geometry/applicators/");
    if (!fileName.isEmpty()) {
        applicPhantPath->setText(fileName);
    }

}


