/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "generalviewsettingspage.h"
#include "dolphinmainwindow.h"
#include "dolphinsettings.h"
#include "dolphinviewcontainer.h"
#include "viewproperties.h"

#include "dolphin_generalsettings.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QBoxLayout>

#include <kconfiggroup.h>
#include <kdialog.h>
#include <kglobal.h>
#include <klocale.h>
#include <khbox.h>

GeneralViewSettingsPage::GeneralViewSettingsPage(const KUrl& url,
                                                 QWidget* parent) :
    ViewSettingsPageBase(parent),
    m_url(url),
    m_localProps(0),
    m_globalProps(0),
    m_maxPreviewSize(0),
    m_spinBox(0),
    m_useFileThumbnails(0),
    m_showSelectionToggle(0)
{
    const int spacing = KDialog::spacingHint();
    const int margin = KDialog::marginHint();
    const QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    setSpacing(spacing);
    setMargin(margin);

    QGroupBox* propsBox = new QGroupBox(i18nc("@title:group", "View Properties"), this);

    m_localProps = new QRadioButton(i18nc("@option:radio", "Remember view properties for each folder"), propsBox);
    connect(m_localProps, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    m_globalProps = new QRadioButton(i18nc("@option:radio", "Use common view properties for all folders"), propsBox);
    connect(m_globalProps, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    QVBoxLayout* propsBoxLayout = new QVBoxLayout(propsBox);
    propsBoxLayout->addWidget(m_localProps);
    propsBoxLayout->addWidget(m_globalProps);

    // create 'File Previews' box
    QGroupBox* previewBox = new QGroupBox(i18nc("@title:group", "File Previews"), this);

    KHBox* vBox = new KHBox(previewBox);
    vBox->setSpacing(spacing);

    new QLabel(i18nc("@label:slider", "Maximum file size:"), vBox);
    m_maxPreviewSize = new QSlider(Qt::Horizontal, vBox);

    m_spinBox = new QSpinBox(vBox);

    connect(m_maxPreviewSize, SIGNAL(valueChanged(int)),
            m_spinBox, SLOT(setValue(int)));
    connect(m_spinBox, SIGNAL(valueChanged(int)),
            m_maxPreviewSize, SLOT(setValue(int)));

    connect(m_maxPreviewSize, SIGNAL(valueChanged(int)),
            this, SIGNAL(changed()));
    connect(m_spinBox, SIGNAL(valueChanged(int)),
            this, SIGNAL(changed()));

    m_useFileThumbnails = new QCheckBox(i18nc("@option:check", "Use thumbnails embedded in files"), previewBox);
    connect(m_useFileThumbnails, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    QVBoxLayout* previewBoxLayout = new QVBoxLayout(previewBox);
    previewBoxLayout->addWidget(vBox);
    previewBoxLayout->addWidget(m_useFileThumbnails);

    m_showSelectionToggle = new QCheckBox(i18nc("@option:check", "Show selection toggle"), this);
    connect(m_showSelectionToggle, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(this);

    loadSettings();
}


GeneralViewSettingsPage::~GeneralViewSettingsPage()
{
}

void GeneralViewSettingsPage::applySettings()
{
    ViewProperties props(m_url);  // read current view properties

    const bool useGlobalProps = m_globalProps->isChecked();

    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    settings->setGlobalViewProps(useGlobalProps);

    if (useGlobalProps) {
        // Remember the global view properties by applying the current view properties.
        // It is important that GeneralSettings::globalViewProps() is set before
        // the class ViewProperties is used, as ViewProperties uses this setting
        // to find the destination folder for storing the view properties.
        ViewProperties globalProps(m_url);
        globalProps.setDirProperties(props);
    }

    KConfigGroup globalConfig(KGlobal::config(), "PreviewSettings");
    const int byteCount = m_maxPreviewSize->value() * 1024 * 1024; // value() returns size in MB
    globalConfig.writeEntry("MaximumSize",
                            byteCount,
                            KConfigBase::Normal | KConfigBase::Global);
    globalConfig.writeEntry("UseFileThumbnails",
                            m_useFileThumbnails->isChecked(),
                            KConfigBase::Normal | KConfigBase::Global);
    globalConfig.sync();

    settings->setShowSelectionToggle(m_showSelectionToggle->isChecked());
}

void GeneralViewSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    settings->setDefaults();
    loadSettings();
}

void GeneralViewSettingsPage::loadSettings()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    if (settings->globalViewProps()) {
        m_globalProps->setChecked(true);
    } else {
        m_localProps->setChecked(true);
    }

    const int min = 1;   // MB
    const int max = 100; // MB
    m_maxPreviewSize->setRange(min, max);
    m_maxPreviewSize->setPageStep(10);
    m_maxPreviewSize->setSingleStep(1);
    m_maxPreviewSize->setTickPosition(QSlider::TicksBelow);

    KConfigGroup globalConfig(KGlobal::config(), "PreviewSettings");
    // TODO: The default value of 5 MB must match with the default value inside
    // kdelibs/kio/kio/previewjob.cpp. Maybe a static getter method in PreviewJob
    // should be added for getting the default size?
    const int maxByteSize = globalConfig.readEntry("MaximumSize", 5 * 1024 * 1024 /* 5 MB */);
    int maxMByteSize = maxByteSize / (1024 * 1024);
    if (maxMByteSize < 1) {
        maxMByteSize = 1;
    } else if (maxMByteSize > max) {
        maxMByteSize = max;
    }

    m_spinBox->setRange(min, max);
    m_spinBox->setSingleStep(1);
    m_spinBox->setSuffix(" MB");

    m_maxPreviewSize->setValue(maxMByteSize);
    m_spinBox->setValue(m_maxPreviewSize->value());

    const bool useFileThumbnails = globalConfig.readEntry("UseFileThumbnails", true);
    m_useFileThumbnails->setChecked(useFileThumbnails);

    m_showSelectionToggle->setChecked(settings->showSelectionToggle());
}

#include "generalviewsettingspage.moc"
