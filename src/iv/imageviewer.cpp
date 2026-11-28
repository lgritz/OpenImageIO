// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause and Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

#include <algorithm>
#include <cmath>
#include <iostream>
#ifndef _WIN32
#    include <unistd.h>
#endif
#include <vector>

#include "imageviewer.h"
#include "ivgl.h"
#include "ivgl_ocio.h"

#ifdef IV_USE_QT
#    include <QMessageBox>
#    include <QMimeData>
#endif

#include <OpenImageIO/color.h>
#include <OpenImageIO/dassert.h>
#include <OpenImageIO/filesystem.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imagecache.h>
#include <OpenImageIO/strutil.h>
#include <OpenImageIO/sysutil.h>
#include <OpenImageIO/timer.h>


#include "ivutils.h"


// clang-format off
static const char *s_file_filters = ""
    "Image Files (*.bmp *.cin *.dcm *.dds *.dpx *.fits *.gif *.hdr *.ico *.iff "
    "*.jpg *.jpe *.jpeg *.jif *.jfif *.jfi *.jp2 *.j2k *.jxl *.exr *.png *.pbm *.pgm "
    "*.ppm *.psd *.ptex *.R3D *.r3d *.rla *.sgi *.rgb *.rgba *.bw *.int *.inta *.pic *.tga "
    "*.tpic *.tif *.tiff *.tx *.env *.sm *.vsm *.vdb *.webp *.zfile);;"
    "BMP (*.bmp);;"
    "Cineon (*.cin);;"
    "Direct Draw Surface (*.dds);;"
    "DICOM (*.dcm);;"
    "DPX (*.dpx);;"
    "FITS (*.fits);;"
    "GIF (*.gif);;"
    "HDR/RGBE (*.hdr);;"
    "Icon (*.ico);;"
    "IFF (*.iff);;"
    "JPEG (*.jpg *.jpe *.jpeg *.jif *.jfif *.jfi);;"
    "JPEG-2000 (*.jp2 *.j2k);;"
    "JPEG XL (*.jxl);;"
    "OpenEXR (*.exr);;"
    "OpenVDB (*.vdb);;"
    "PhotoShop (*.psd);;"
    "Portable Network Graphics (*.png);;"
    "PNM / Netpbm (*.pbm *.pgm *.ppm);;"
    "Ptex (*.ptex);;"
    "R3D (*.R3D *.r3d);;"
    "RLA (*.rla);;"
    "SGI (*.sgi *.rgb *.rgba *.bw *.int *.inta);;"
    "Softimage PIC (*.pic);;"
    "Targa (*.tga *.tpic);;"
    "TIFF (*.tif *.tiff *.tx *.env *.sm *.vsm);;"
    "Webp (*.webp);;"
    "Zfile (*.zfile);;"
    "All Files (*)";
// clang-format on



ImageViewer::ImageViewer(bool use_ocio, const std::string& image_color_space,
                         const std::string& display, const std::string& view)
    : infoWindow(NULL)
    , preferenceWindow(NULL)
    , darkPaletteBox(NULL)
    , m_current_image(-1)
    , m_current_channel(0)
    , m_color_mode(RGBA)
    , m_last_image(-1)
    , m_zoom(1)
    , m_fullscreen(false)
    , m_default_gamma(1)
    , m_darkPalette(false)
    , m_useOCIO(use_ocio)
    , m_ocioColourSpace(image_color_space)
    , m_ocioDisplay(display)
    , m_ocioView(view)
{
    readSettings(false);

    float gam = Strutil::stof(Sysutil::getenv("GAMMA", "2.2"));
    if (gam >= 0.1 && gam <= 5)
        m_default_gamma = gam;
    // FIXME -- would be nice to have a more nuanced approach to display
    // color space, in particular knowing whether the display is sRGB.
    // Also, some time in the future we may want a real 3D LUT for
    // "film look", etc.

#ifdef IV_USE_QT
    if (darkPalette())
        m_palette = QPalette(Qt::darkGray);  // darkGray?
    else
        m_palette = QPalette();
    QApplication::setPalette(m_palette);  // FIXME -- why not work?
    this->setPalette(m_palette);
#else
    iv_set_dark_style(darkPalette());
#endif

    slideTimer       = new IvTimer();
    slideDuration_ms = 5000;
    slide_loop       = true;

    glwin = new IvGL_OCIO(this, *this);

#ifdef IV_USE_QT
    glwin->setPalette(m_palette);
#endif
    glwin->resize(m_default_width, m_default_height);
    setCentralWidget(glwin);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    readSettings();

    setWindowTitle(tr("Image Viewer"));
    resize(m_default_width, m_default_height);

#ifdef IV_USE_QT
    setAcceptDrops(true);
    // Disable drag and drop on child widgets
    for (QWidget* child : findChildren<QWidget*>()) {
        child->setAcceptDrops(false);
    }
    setAttribute(Qt::WA_DeleteOnClose);
#endif
}



ImageViewer::~ImageViewer()
{
    // Wait for a still-running FLIP diff; it only touches local copies.
    if (m_flip_thread.joinable())
        m_flip_thread.join();
    for (auto i : m_images)
        delete i;
}



void
ImageViewer::closeEvent(IvCloseEvent*)
{
    writeSettings();
}

void
ImageViewer::add_dropped_files(const std::vector<std::string>& filenames)
{
    size_t old_size = m_images.size();
    for (auto& f : filenames)
        add_image(f);
    // Switch to the first newly added image
    if (m_images.size() > old_size)
        current_image(old_size);
}



#ifdef IV_USE_QT
void
ImageViewer::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void
ImageViewer::dropEvent(QDropEvent* event)
{
    std::vector<std::string> filenames;
    for (const QUrl& url : event->mimeData()->urls())
        filenames.emplace_back(url.toLocalFile().toStdString());
    add_dropped_files(filenames);
}
#endif



void
ImageViewer::createActions()
{
    openAct = new IvAction(tr("&Open..."), this);
    openAct->setShortcut(tr("Ctrl+O"));
    connect(openAct, &IvAction::triggered, this, &ImageViewer::open);

    for (size_t i = 0; i < MaxRecentFiles; ++i) {
        openRecentAct[i] = new IvAction(this);
        openRecentAct[i]->setVisible(false);
        connect(openRecentAct[i], &IvAction::triggered, this,
                [this, i] { openRecentFile(i); });
    }

    reloadAct = new IvAction(tr("&Reload image"), this);
    reloadAct->setShortcut(tr("Ctrl+R"));
    connect(reloadAct, &IvAction::triggered, this, &ImageViewer::reload);

    closeImgAct = new IvAction(tr("&Close Image"), this);
    closeImgAct->setShortcut(tr("Ctrl+W"));
    connect(closeImgAct, &IvAction::triggered, this, &ImageViewer::closeImg);

    saveAsAct = new IvAction(tr("&Save As..."), this);
    saveAsAct->setShortcut(tr("Ctrl+S"));
    connect(saveAsAct, &IvAction::triggered, this, &ImageViewer::saveAs);

    saveWindowAsAct = new IvAction(tr("Save Window As..."), this);
    connect(saveWindowAsAct, &IvAction::triggered, this,
            &ImageViewer::saveWindowAs);

    saveSelectionAsAct = new IvAction(tr("Save Selection As..."), this);
    connect(saveSelectionAsAct, &IvAction::triggered, this,
            &ImageViewer::saveSelectionAs);

    moveToNewWindowAct = new IvAction(tr("Move to new window"), this);
    connect(moveToNewWindowAct, &IvAction::triggered, this,
            &ImageViewer::moveToNewWindow);

    deleteCurrentImageAct = new IvAction(tr("&Delete from disk"), this);
    deleteCurrentImageAct->setShortcut(tr("Delete"));
    connect(deleteCurrentImageAct, &IvAction::triggered, this,
            &ImageViewer::deleteCurrentImage);

    editPreferencesAct = new IvAction(tr("&Preferences..."), this);
    editPreferencesAct->setShortcut(tr("Ctrl+,"));
    editPreferencesAct->setEnabled(true);
    connect(editPreferencesAct, &IvAction::triggered, this,
            &ImageViewer::editPreferences);

    exitAct = new IvAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    connect(exitAct, &IvAction::triggered, this, &ImageViewer::close);

    exposurePlusOneTenthStopAct = new IvAction(tr("Exposure +1/10 stop"), this);
    exposurePlusOneTenthStopAct->setShortcut(tr("]"));
    connect(exposurePlusOneTenthStopAct, &IvAction::triggered, this,
            &ImageViewer::exposurePlusOneTenthStop);

    exposurePlusOneHalfStopAct = new IvAction(tr("Exposure +1/2 stop"), this);
    exposurePlusOneHalfStopAct->setShortcut(tr("}"));
    connect(exposurePlusOneHalfStopAct, &IvAction::triggered, this,
            &ImageViewer::exposurePlusOneHalfStop);

    exposureMinusOneTenthStopAct = new IvAction(tr("Exposure -1/10 stop"),
                                                this);
    exposureMinusOneTenthStopAct->setShortcut(tr("["));
    connect(exposureMinusOneTenthStopAct, &IvAction::triggered, this,
            &ImageViewer::exposureMinusOneTenthStop);

    exposureMinusOneHalfStopAct = new IvAction(tr("Exposure -1/2 stop"), this);
    exposureMinusOneHalfStopAct->setShortcut(tr("{"));
    connect(exposureMinusOneHalfStopAct, &IvAction::triggered, this,
            &ImageViewer::exposureMinusOneHalfStop);

    gammaPlusAct = new IvAction(tr("Gamma +0.1"), this);
    gammaPlusAct->setShortcut(tr(")"));
    connect(gammaPlusAct, &IvAction::triggered, this, &ImageViewer::gammaPlus);

    gammaMinusAct = new IvAction(tr("Gamma -0.1"), this);
    gammaMinusAct->setShortcut(tr("("));
    connect(gammaMinusAct, &IvAction::triggered, this,
            &ImageViewer::gammaMinus);

    viewChannelFullAct = new IvAction(tr("Full Color"), this);
    viewChannelFullAct->setShortcut(tr("c"));
    viewChannelFullAct->setCheckable(true);
    viewChannelFullAct->setChecked(true);
    connect(viewChannelFullAct, &IvAction::triggered, this,
            &ImageViewer::viewChannelFull);

    viewChannelRedAct = new IvAction(tr("Red"), this);
    viewChannelRedAct->setShortcut(tr("r"));
    viewChannelRedAct->setCheckable(true);
    connect(viewChannelRedAct, &IvAction::triggered, this,
            &ImageViewer::viewChannelRed);

    viewChannelGreenAct = new IvAction(tr("Green"), this);
    viewChannelGreenAct->setShortcut(tr("g"));
    viewChannelGreenAct->setCheckable(true);
    connect(viewChannelGreenAct, &IvAction::triggered, this,
            &ImageViewer::viewChannelGreen);

    viewChannelBlueAct = new IvAction(tr("Blue"), this);
    viewChannelBlueAct->setShortcut(tr("b"));
    viewChannelBlueAct->setCheckable(true);
    connect(viewChannelBlueAct, &IvAction::triggered, this,
            &ImageViewer::viewChannelBlue);

    viewChannelAlphaAct = new IvAction(tr("Alpha"), this);
    viewChannelAlphaAct->setShortcut(tr("a"));
    viewChannelAlphaAct->setCheckable(true);
    connect(viewChannelAlphaAct, &IvAction::triggered, this,
            &ImageViewer::viewChannelAlpha);

    viewColorLumAct = new IvAction(tr("Luminance"), this);
    viewColorLumAct->setShortcut(tr("l"));
    viewColorLumAct->setCheckable(true);
    connect(viewColorLumAct, &IvAction::triggered, this,
            &ImageViewer::viewChannelLuminance);

    viewColorRGBAAct = new IvAction(tr("RGBA"), this);
    //viewColorRGBAAct->setShortcut (tr("Ctrl+c"));
    viewColorRGBAAct->setCheckable(true);
    viewColorRGBAAct->setChecked(true);
    connect(viewColorRGBAAct, &IvAction::triggered, this,
            &ImageViewer::viewColorRGBA);

    viewColorRGBAct = new IvAction(tr("RGB"), this);
    //viewColorRGBAct->setShortcut (tr("Ctrl+a"));
    viewColorRGBAct->setCheckable(true);
    connect(viewColorRGBAct, &IvAction::triggered, this,
            &ImageViewer::viewColorRGB);

    viewColor1ChAct = new IvAction(tr("Single channel"), this);
    viewColor1ChAct->setShortcut(tr("1"));
    viewColor1ChAct->setCheckable(true);
    connect(viewColor1ChAct, &IvAction::triggered, this,
            &ImageViewer::viewColor1Ch);

    viewColorHeatmapAct = new IvAction(tr("Single channel (Heatmap)"), this);
    viewColorHeatmapAct->setShortcut(tr("h"));
    viewColorHeatmapAct->setCheckable(true);
    connect(viewColorHeatmapAct, &IvAction::triggered, this,
            &ImageViewer::viewColorHeatmap);

    viewChannelPrevAct = new IvAction(tr("Prev Channel"), this);
    viewChannelPrevAct->setShortcut(tr(","));
    connect(viewChannelPrevAct, &IvAction::triggered, this,
            &ImageViewer::viewChannelPrev);

    viewChannelNextAct = new IvAction(tr("Next Channel"), this);
    viewChannelNextAct->setShortcut(tr("."));
    connect(viewChannelNextAct, &IvAction::triggered, this,
            &ImageViewer::viewChannelNext);

    viewSubimagePrevAct = new IvAction(tr("Prev Subimage"), this);
    viewSubimagePrevAct->setShortcut(tr("<"));
    connect(viewSubimagePrevAct, &IvAction::triggered, this,
            &ImageViewer::viewSubimagePrev);

    viewSubimageNextAct = new IvAction(tr("Next Subimage"), this);
    viewSubimageNextAct->setShortcut(tr(">"));
    connect(viewSubimageNextAct, &IvAction::triggered, this,
            &ImageViewer::viewSubimageNext);

    zoomInAct = new IvAction(tr("Zoom &In"), this);
    zoomInAct->setShortcut(tr("Ctrl++"));
    connect(zoomInAct, &IvAction::triggered, this, [this] { zoomIn(); });

    zoomOutAct = new IvAction(tr("Zoom &Out"), this);
    zoomOutAct->setShortcut(tr("Ctrl+-"));
    connect(zoomOutAct, &IvAction::triggered, this, [this] { zoomOut(); });

    normalSizeAct = new IvAction(tr("&Normal Size (1:1)"), this);
    normalSizeAct->setShortcut(tr("Ctrl+0"));
    connect(normalSizeAct, &IvAction::triggered, this,
            &ImageViewer::normalSize);

    fitWindowToImageAct = new IvAction(tr("&Fit Window to Image"), this);
    fitWindowToImageAct->setShortcut(tr("f"));
    connect(fitWindowToImageAct, &IvAction::triggered, this,
            [this] { fitWindowToImage(); });

    fitImageToWindowAct = new IvAction(tr("Fit Image to Window"), this);
    //    fitImageToWindowAct->setEnabled(false);
    fitImageToWindowAct->setCheckable(true);
    fitImageToWindowAct->setShortcut(tr("Alt+f"));
    connect(fitImageToWindowAct, &IvAction::triggered, this,
            &ImageViewer::fitImageToWindow);

    fullScreenAct = new IvAction(tr("Full screen"), this);
    //    fullScreenAct->setEnabled(false);
    //    fullScreenAct->setCheckable(true);
    fullScreenAct->setShortcut(tr("Ctrl+f"));
    connect(fullScreenAct, &IvAction::triggered, this,
            &ImageViewer::fullScreenToggle);

    aboutAct = new IvAction(tr("&About"), this);
    connect(aboutAct, &IvAction::triggered, this, &ImageViewer::about);

    formatsAct = new IvAction(tr("Image &Formats..."), this);
    connect(formatsAct, &IvAction::triggered, this,
            &ImageViewer::showImageFormats);

    prevImageAct = new IvAction(tr("Previous Image"), this);
    prevImageAct->setShortcut(tr("PgUp"));
    //    prevImageAct->setEnabled(true);
    connect(prevImageAct, &IvAction::triggered, this, &ImageViewer::prevImage);

    nextImageAct = new IvAction(tr("Next Image"), this);
    nextImageAct->setShortcut(tr("PgDown"));
    //    nextImageAct->setEnabled(true);
    connect(nextImageAct, &IvAction::triggered, this, &ImageViewer::nextImage);

    toggleImageAct = new IvAction(tr("Toggle image"), this);
    toggleImageAct->setShortcut(tr("T"));
    //    toggleImageAct->setEnabled(true);
    connect(toggleImageAct, &IvAction::triggered, this,
            &ImageViewer::toggleImage);

    comparisonAct = new IvAction(tr("Wipe comparison"), this);
    comparisonAct->setCheckable(true);
    comparisonAct->setShortcut(tr("W"));
    connect(comparisonAct, &IvAction::toggled, this,
            &ImageViewer::toggleComparison);

    // Difference view submenu (issue #4711): a plain difference view of the
    // two images, kept separate from the wipe comparison (#5480).
    differenceMenu   = new IvMenu(tr("&Difference"), this);
    auto* diffGroup  = new IvActionGroup(this);
    differenceOffAct = new IvAction(tr("&Off"), this);
    differenceOffAct->setCheckable(true);
    differenceOffAct->setChecked(true);
    differenceAbsDiffAct = new IvAction(tr("&Absolute difference"), this);
    differenceAbsDiffAct->setCheckable(true);
    differenceFlipDiffAct = new IvAction(tr("&FLIP perceptual difference"),
                                         this);
    differenceFlipDiffAct->setCheckable(true);
    diffGroup->addAction(differenceOffAct);
    diffGroup->addAction(differenceAbsDiffAct);
    diffGroup->addAction(differenceFlipDiffAct);
    connect(differenceOffAct, &IvAction::toggled, this,
            &ImageViewer::differenceModeChanged);
    connect(differenceAbsDiffAct, &IvAction::toggled, this,
            &ImageViewer::differenceModeChanged);
    connect(differenceFlipDiffAct, &IvAction::toggled, this,
            &ImageViewer::differenceModeChanged);
    differenceCycleAct = new IvAction(tr("Cycle difference view"), this);
    differenceCycleAct->setShortcut(tr("D"));
    connect(differenceCycleAct, &IvAction::triggered, this,
            &ImageViewer::cycleDifference);
    flipColormapAct = new IvAction(tr("Color&map FLIP error map"), this);
    flipColormapAct->setCheckable(true);
    flipColormapAct->setChecked(true);
    connect(flipColormapAct, &IvAction::toggled, this,
            &ImageViewer::flipColormapToggled);

    toggleWindowGuidesAct
        = new IvAction(tr("Show display and data window borders"), this);
    toggleWindowGuidesAct->setCheckable(true);
    connect(toggleWindowGuidesAct, &IvAction::triggered, this,
            &ImageViewer::toggleWindowGuides);

    slideShowAct = new IvAction(tr("Start Slide Show"), this);
    connect(slideShowAct, &IvAction::triggered, this, &ImageViewer::slideShow);

    slideLoopAct = new IvAction(tr("Loop slide show"), this);
    slideLoopAct->setCheckable(true);
    slideLoopAct->setChecked(true);
    connect(slideLoopAct, &IvAction::triggered, this, &ImageViewer::slideLoop);

    slideNoLoopAct = new IvAction(tr("Stop at end"), this);
    slideNoLoopAct->setCheckable(true);
    connect(slideNoLoopAct, &IvAction::triggered, this,
            &ImageViewer::slideNoLoop);

    sortByNameAct = new IvAction(tr("By Name"), this);
    connect(sortByNameAct, &IvAction::triggered, this,
            &ImageViewer::sortByName);

    sortByPathAct = new IvAction(tr("By File Path"), this);
    connect(sortByPathAct, &IvAction::triggered, this,
            &ImageViewer::sortByPath);

    sortByImageDateAct = new IvAction(tr("By Image Date"), this);
    connect(sortByImageDateAct, &IvAction::triggered, this,
            &ImageViewer::sortByImageDate);

    sortByFileDateAct = new IvAction(tr("By File Date"), this);
    connect(sortByFileDateAct, &IvAction::triggered, this,
            &ImageViewer::sortByFileDate);

    sortReverseAct = new IvAction(tr("Reverse current order"), this);
    connect(sortReverseAct, &IvAction::triggered, this,
            &ImageViewer::sortReverse);

    showInfoWindowAct = new IvAction(tr("&Image info..."), this);
    showInfoWindowAct->setShortcut(tr("Ctrl+I"));
    //    showInfoWindowAct->setEnabled(true);
    connect(showInfoWindowAct, &IvAction::triggered, this,
            &ImageViewer::showInfoWindow);

    showPixelviewWindowAct = new IvAction(tr("&Pixel closeup view..."), this);
    showPixelviewWindowAct->setCheckable(true);
    showPixelviewWindowAct->setShortcut(tr("P"));
    connect(showPixelviewWindowAct, &IvAction::triggered, this,
            &ImageViewer::showPixelviewWindow);

    pixelviewFollowsMouseBox = new IvCheckBox(tr("Pixel view follows mouse"));
    pixelviewFollowsMouseBox->setChecked(false);
    linearInterpolationBox = new IvCheckBox(tr("Linear interpolation"));
    linearInterpolationBox->setChecked(true);
    darkPaletteBox = new IvCheckBox(tr("Dark palette"));
    darkPaletteBox->setChecked(true);
    autoMipmap = new IvCheckBox(tr("Generate mipmaps (requires restart)"));
    autoMipmap->setChecked(false);

    maxMemoryICLabel = new IvLabel(
        tr("Image Cache max memory (requires restart)"));
    maxMemoryIC = new IvSpinBox();
    if (sizeof(void*) == 4)
        maxMemoryIC->setRange(128, 2048);  //2GB is enough for 32 bit machines
    else
        maxMemoryIC->setRange(128, 8192);  //8GB probably ok for 64 bit
    maxMemoryIC->setSingleStep(64);
    maxMemoryIC->setSuffix(" MB");

    slideShowDurationLabel = new IvLabel(tr("Slide Show delay"));
    slideShowDuration      = new IvSpinBox();
    slideShowDuration->setRange(1, 3600);
    slideShowDuration->setSingleStep(1);
    slideShowDuration->setSuffix(" s");
    slideShowDuration->setAccelerated(true);
    connect(slideShowDuration, IV_SPIN_VALUE_CHANGED, this,
            &ImageViewer::setSlideShowDuration);

    toggleAreaSampleAct = new IvAction(tr("&Toggle Area Sample"), this);
    toggleAreaSampleAct->setCheckable(true);
    toggleAreaSampleAct->setShortcut(tr("Ctrl+A"));
    connect(toggleAreaSampleAct, &IvAction::triggered, this,
            &ImageViewer::toggleAreaSample);

    closeupPixelsLabel = new IvLabel(tr("# closeup pixels:"));
    closeupPixelsBox   = new IvSpinBox();
    closeupPixelsBox->setRange(9, 25);
    closeupPixelsBox->setValue(13);
    closeupPixelsBox->setSingleStep(2);

    auto closeupPixelsTooltip = tr(
        "Number of pixels to show in the closeup view.");
    closeupPixelsBox->setToolTip(closeupPixelsTooltip);
    closeupPixelsLabel->setToolTip(closeupPixelsTooltip);

    closeupAvgPixelsLabel = new IvLabel(tr("# closeup avg pixels:"));
    closeupAvgPixelsBox   = new IvSpinBox();
    closeupAvgPixelsBox->setRange(3, 25);
    closeupAvgPixelsBox->setValue(11);
    closeupAvgPixelsBox->setSingleStep(2);

    auto closeupAvgPixelsTooltip = tr(
        "Number of pixels to use for averaging in the closeup view.");
    closeupAvgPixelsBox->setToolTip(closeupAvgPixelsTooltip);
    closeupAvgPixelsLabel->setToolTip(closeupAvgPixelsTooltip);


    rotateLeftAct = new IvAction(tr("&Rotate Left"), this);
    rotateLeftAct->setShortcut(tr("Ctrl+Shift+L"));
    connect(rotateLeftAct, &IvAction::triggered, this,
            &ImageViewer::rotateLeft);

    rotateRightAct = new IvAction(tr("&Rotate Right"), this);
    rotateRightAct->setShortcut(tr("Ctrl+Shift+R"));
    connect(rotateRightAct, &IvAction::triggered, this,
            &ImageViewer::rotateRight);

    flipHorizontalAct = new IvAction(tr("&Flip Horizontal"), this);
    connect(flipHorizontalAct, &IvAction::triggered, this,
            &ImageViewer::flipHorizontal);

    flipVerticalAct = new IvAction(tr("&Flip Vertical"), this);
    connect(flipVerticalAct, &IvAction::triggered, this,
            &ImageViewer::flipVertical);

    // Connect signals to ensure closeupAvgPixelsBox value is always <= closeupPixelsBox value
    connect(closeupPixelsBox, IV_SPIN_VALUE_CHANGED, [this](int value) {
        if (closeupAvgPixelsBox->value() > value)
            closeupAvgPixelsBox->setValue(value);
    });
    connect(closeupAvgPixelsBox, IV_SPIN_VALUE_CHANGED, [this](int value) {
        if (value > closeupPixelsBox->value())
            closeupAvgPixelsBox->setValue(closeupPixelsBox->value());
    });
}



void
ImageViewer::createOCIOMenus(IvMenu* parent)
{
    ocioColorSpacesMenu = new IvMenu(tr("Image color space"));
    ocioDisplaysMenu    = new IvMenu(tr("Display/View"));

    try {
        ColorConfig config;

        std::map<std::string, IvMenu*> colourSpaceFamilies;

        ocioColorSpacesGroup = new IvActionGroup(ocioColorSpacesMenu);
        ocioColorSpacesGroup->setExclusive(true);

        for (int i = 0; i < config.getNumColorSpaces(); i++) {
            const char* colorSpaceName = config.getColorSpaceNameByIndex(i);

            if (colorSpaceName && *colorSpaceName
                && config.isColorSpaceActive(colorSpaceName)) {
                // If no color space provided via command line parameters,
                // select the top active color space in the list.
                if (m_ocioColourSpace == "") {
                    m_ocioColourSpace = colorSpaceName;
                }

                const char* family = config.getColorSpaceFamilyByName(
                    colorSpaceName);

                IvMenu* targetMenu;
                if (family && *family) {
                    auto iter = colourSpaceFamilies.find(family);
                    if (iter == colourSpaceFamilies.end()) {
                        targetMenu = new IvMenu(family);
                        ocioColorSpacesMenu->addMenu(targetMenu);
                        colourSpaceFamilies[family] = targetMenu;
                    } else {
                        targetMenu = iter->second;
                    }
                } else {
                    targetMenu = ocioColorSpacesMenu;
                }

                IvAction* action = new IvAction(colorSpaceName, this);
                action->setCheckable(true);
                action->setChecked(m_ocioColourSpace == colorSpaceName);

                connect(action, &IvAction::triggered, this,
                        [this, name = std::string(colorSpaceName)] {
                            ocioColorSpaceAction(name);
                        });

                ocioColorSpacesGroup->addAction(action);
                targetMenu->addAction(action);
            }
        }

        ocioDisplayViewsGroup = new IvActionGroup(ocioDisplaysMenu);
        ocioDisplayViewsGroup->setExclusive(true);

        if (m_ocioDisplay == "" || m_ocioDisplay == "default") {
            m_ocioDisplay = config.getDefaultDisplayName();
        }

        if (m_ocioView == "" || m_ocioView == "default") {
            m_ocioView = config.getDefaultViewName();
        }

        for (int i = 0; i < config.getNumDisplays(); i++) {
            const char* display = config.getDisplayNameByIndex(i);

            if (display && *display) {
                IvMenu* menu = new IvMenu(display);

                for (int j = 0; j < config.getNumViews(display); j++) {
                    const char* view = config.getViewNameByIndex(display, j);

                    if (view && *view) {
                        IvAction* action = new IvAction(view, menu);
                        action->setCheckable(true);
                        action->setChecked(m_ocioDisplay == display
                                           && m_ocioView == view);

                        connect(action, &IvAction::triggered, this,
                                [this, d = std::string(display),
                                 v = std::string(view)] {
                                    ocioDisplayViewAction(d, v);
                                });

                        menu->addAction(action);
                        ocioDisplayViewsGroup->addAction(action);
                    }
                }

                ocioDisplaysMenu->addMenu(menu);
            }
        }
    } catch (...) {
        std::cerr << "Error loading OCIO config file" << std::endl;
        m_useOCIO = false;
        return;
    }

    IvMenu* ocioMenu = new IvMenu(tr("OCIO"));

    IvAction* action = new IvAction(tr("Use OCIO"));
    action->setCheckable(true);
    action->setChecked(m_useOCIO);
    connect(action, &IvAction::toggled, this, &ImageViewer::useOCIOAction);

    ocioMenu->addAction(action);
    ocioMenu->addMenu(ocioColorSpacesMenu);
    ocioMenu->addMenu(ocioDisplaysMenu);

    parent->addMenu(ocioMenu);
}

void
ImageViewer::useOCIOAction(bool checked)
{
    m_useOCIO = checked;

    ocioColorSpacesMenu->setEnabled(m_useOCIO);
    ocioDisplaysMenu->setEnabled(m_useOCIO);

    displayCurrentImage();
}

void
ImageViewer::ocioColorSpaceAction(const std::string& name)
{
    m_ocioColourSpace = name;
    displayCurrentImage();
}

void
ImageViewer::ocioDisplayViewAction(const std::string& display,
                                   const std::string& view)
{
    m_ocioDisplay = display;
    m_ocioView    = view;
    displayCurrentImage();
}


void
ImageViewer::createMenus()
{
    openRecentMenu = new IvMenu(tr("Open recent..."), this);
    for (auto& i : openRecentAct)
        openRecentMenu->addAction(i);

    fileMenu = new IvMenu(tr("&File"), this);
    fileMenu->addAction(openAct);
    fileMenu->addMenu(openRecentMenu);
    fileMenu->addAction(reloadAct);
    fileMenu->addAction(closeImgAct);
    fileMenu->addSeparator();
    fileMenu->addAction(saveAsAct);
    fileMenu->addAction(saveWindowAsAct);
    fileMenu->addAction(saveSelectionAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(moveToNewWindowAct);
    fileMenu->addAction(deleteCurrentImageAct);
    fileMenu->addSeparator();
    fileMenu->addAction(editPreferencesAct);
    fileMenu->addAction(exitAct);
    menuBar()->addMenu(fileMenu);

    // Copy
    // Paste
    // Clear selection
    // radio: prioritize selection, crop selection

    expgamMenu = new IvMenu(tr("Exposure/gamma"));  // submenu
    expgamMenu->addAction(exposureMinusOneHalfStopAct);
    expgamMenu->addAction(exposureMinusOneTenthStopAct);
    expgamMenu->addAction(exposurePlusOneHalfStopAct);
    expgamMenu->addAction(exposurePlusOneTenthStopAct);
    expgamMenu->addAction(gammaMinusAct);
    expgamMenu->addAction(gammaPlusAct);

    //    imageMenu = new IvMenu(tr("&Image"), this);
    //    menuBar()->addMenu (imageMenu);
    slideMenu = new IvMenu(tr("Slide Show"));
    slideMenu->addAction(slideShowAct);
    slideMenu->addAction(slideLoopAct);
    slideMenu->addAction(slideNoLoopAct);

    sortMenu = new IvMenu(tr("Sort"));
    sortMenu->addAction(sortByNameAct);
    sortMenu->addAction(sortByPathAct);
    sortMenu->addAction(sortByImageDateAct);
    sortMenu->addAction(sortByFileDateAct);
    sortMenu->addAction(sortReverseAct);

    channelMenu = new IvMenu(tr("Channels"));
    // Color mode: true, random, falsegrgbacCrgR
    channelMenu->addAction(viewChannelFullAct);
    channelMenu->addAction(viewChannelRedAct);
    channelMenu->addAction(viewChannelGreenAct);
    channelMenu->addAction(viewChannelBlueAct);
    channelMenu->addAction(viewChannelAlphaAct);
    channelMenu->addAction(viewChannelPrevAct);
    channelMenu->addAction(viewChannelNextAct);

    colormodeMenu = new IvMenu(tr("Color mode"));
    colormodeMenu->addAction(viewColorRGBAAct);
    colormodeMenu->addAction(viewColorRGBAct);
    colormodeMenu->addAction(viewColor1ChAct);
    colormodeMenu->addAction(viewColorLumAct);
    colormodeMenu->addAction(viewColorHeatmapAct);

    viewMenu = new IvMenu(tr("&View"), this);
    viewMenu->addAction(prevImageAct);
    viewMenu->addAction(nextImageAct);
    viewMenu->addAction(toggleImageAct);
    viewMenu->addAction(comparisonAct);
    differenceMenu->addAction(differenceOffAct);
    differenceMenu->addAction(differenceAbsDiffAct);
    differenceMenu->addAction(differenceFlipDiffAct);
    differenceMenu->addSeparator();
    differenceMenu->addAction(flipColormapAct);
    differenceMenu->addSeparator();
    differenceMenu->addAction(differenceCycleAct);
    viewMenu->addMenu(differenceMenu);
    viewMenu->addAction(toggleWindowGuidesAct);
    viewMenu->addSeparator();
    viewMenu->addAction(zoomInAct);
    viewMenu->addAction(zoomOutAct);
    viewMenu->addAction(normalSizeAct);
    viewMenu->addAction(fitWindowToImageAct);
    viewMenu->addAction(fitImageToWindowAct);
    viewMenu->addAction(fullScreenAct);
    viewMenu->addSeparator();
    viewMenu->addAction(viewSubimagePrevAct);
    viewMenu->addAction(viewSubimageNextAct);
    viewMenu->addMenu(channelMenu);
    viewMenu->addMenu(colormodeMenu);

    createOCIOMenus(viewMenu);

    viewMenu->addMenu(expgamMenu);
    menuBar()->addMenu(viewMenu);
    // Full screen mode
    // prev subimage <, next subimage >
    // fg/bg image...

    toolsMenu = new IvMenu(tr("&Tools"), this);
    // Mode: select, zoom, pan, wipe
    toolsMenu->addAction(showInfoWindowAct);
    toolsMenu->addAction(showPixelviewWindowAct);
    toolsMenu->addAction(toggleAreaSampleAct);
    toolsMenu->addMenu(slideMenu);
    toolsMenu->addMenu(sortMenu);
    toolsMenu->addSeparator();
    toolsMenu->addAction(rotateLeftAct);
    toolsMenu->addAction(rotateRightAct);
    toolsMenu->addAction(flipHorizontalAct);
    toolsMenu->addAction(flipVerticalAct);

    // Menus, toolbars, & status
    // Annotate
    // [check] overwrite render
    // connect renderer
    // kill renderer
    // store render
    // Playback: forward, reverse, faster, slower, loop/pingpong
    menuBar()->addMenu(toolsMenu);

    helpMenu = new IvMenu(tr("&Help"), this);
    helpMenu->addAction(formatsAct);
    helpMenu->addSeparator();
    helpMenu->addAction(aboutAct);
    menuBar()->addMenu(helpMenu);
    // Bring up user's guide
}



void
ImageViewer::createToolBars()
{
#if 0
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);

    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addAction(cutAct);
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);
#endif
}



void
ImageViewer::createStatusBar()
{
    statusImgInfo = new IvLabel;
    statusBar()->addWidget(statusImgInfo);

    statusViewInfo = new IvLabel;
    statusBar()->addWidget(statusViewInfo);

    statusProgress = new IvProgressBar;
    statusProgress->setRange(0, 100);
    statusProgress->reset();
    statusBar()->addWidget(statusProgress);

    mouseModeComboBox = new IvComboBox;
    mouseModeComboBox->addItem(tr("Zoom"));
    mouseModeComboBox->addItem(tr("Pan"));
    mouseModeComboBox->addItem(tr("Select"));
    mouseModeComboBox->addItem(tr("Annotate"));
    // Note: the order of the above MUST match the order of enum MouseMode
    statusBar()->addWidget(mouseModeComboBox);
    mouseModeComboBox->hide();
}



void
ImageViewer::readSettings(bool ui_is_set_up)
{
    IvSettings settings("OpenImageIO", "iv");
    m_darkPalette = settings.value("darkPalette").toBool();
    if (!ui_is_set_up)
        return;
    pixelviewFollowsMouseBox->setChecked(
        settings.value("pixelviewFollowsMouse").toBool());
    linearInterpolationBox->setChecked(
        settings.value("linearInterpolation").toBool());
    darkPaletteBox->setChecked(settings.value("darkPalette").toBool());
    IvStringList recent = settings.value("RecentFiles").toStringList();
    for (auto&& s : recent)
        addRecentFile(iv_str(s));
    updateRecentFilesMenu();  // only safe because it's called after menu setup

    autoMipmap->setChecked(settings.value("autoMipmap", false).toBool());
    if (sizeof(void*) == 4)  // 32 bit or 64?
        maxMemoryIC->setValue(settings.value("maxMemoryIC", 512).toInt());
    else
        maxMemoryIC->setValue(settings.value("maxMemoryIC", 2048).toInt());
    slideShowDuration->setValue(
        settings.value("slideShowDuration", 10).toInt());
    closeupPixelsBox->setValue(settings.value("closeupPixels", 13).toInt());

    OIIO::attribute("imagebuf:use_imagecache", 1);

    auto imagecache = ImageCache::create(true);
    imagecache->attribute("automip", autoMipmap->isChecked());
    imagecache->attribute("max_memory_MB", (float)maxMemoryIC->value());
}



void
ImageViewer::writeSettings()
{
    IvSettings settings("OpenImageIO", "iv");
    settings.setValue("pixelviewFollowsMouse",
                      pixelviewFollowsMouseBox->isChecked());
    settings.setValue("linearInterpolation",
                      linearInterpolationBox->isChecked());
    settings.setValue("darkPalette", darkPaletteBox->isChecked());
    settings.setValue("autoMipmap", autoMipmap->isChecked());
    settings.setValue("maxMemoryIC", maxMemoryIC->value());
    settings.setValue("slideShowDuration", slideShowDuration->value());
    settings.setValue("closeupPixels", closeupPixelsBox->value());
    IvStringList recent;
    for (auto&& s : m_recent_files)
        recent.push_front(s.c_str());
    settings.setValue("RecentFiles", recent);
}



bool
image_progress_callback(void* opaque, float done)
{
    ImageViewer* viewer = (ImageViewer*)opaque;
    viewer->statusProgress->setValue((int)(done * 100));
    IvApplication::processEvents();
    return false;
}



void
ImageViewer::open()
{
    static std::string openPath;
    auto names = iv_open_files(this, "Open File(s)", openPath, s_file_filters);
    if (names.empty())
        return;

    int old_lastimage = m_images.size() - 1;
    for (auto& filename : names) {
        if (filename.empty())
            continue;
        add_image(filename);
    }

    if (old_lastimage >= 0) {
        // Otherwise, add_image already did this for us.
        current_image(old_lastimage + 1);
    }
}



void
ImageViewer::openRecentFile(size_t index)
{
    if (index < m_recent_files.size()) {
        std::string filename = m_recent_files[index];
        // If it's an image we already have loaded, just switch to it
        // (and reload) rather than loading a second copy.
        for (size_t i = 0; i < m_images.size(); ++i) {
            if (m_images[i]->name() == filename) {
                current_image(i);
                reload();
                return;
            }
        }
        // It's not an image we already have loaded
        add_image(filename);
        if (m_images.size() > 1) {
            // Otherwise, add_image already did this for us.
            current_image(m_images.size() - 1);
        }
    }
}



void
ImageViewer::addRecentFile(const std::string& name)
{
    removeRecentFile(name);
    m_recent_files.insert(m_recent_files.begin(), name);
    if (m_recent_files.size() > MaxRecentFiles)
        m_recent_files.resize(MaxRecentFiles);
}



void
ImageViewer::removeRecentFile(const std::string& name)
{
    for (size_t i = 0; i < m_recent_files.size(); ++i) {
        if (m_recent_files[i] == name) {
            m_recent_files.erase(m_recent_files.begin() + i);
            --i;
        }
    }
}



void
ImageViewer::updateRecentFilesMenu()
{
    for (size_t i = 0; i < MaxRecentFiles; ++i) {
        if (i < m_recent_files.size()) {
            std::string name = Filesystem::filename(m_recent_files[i]);
            openRecentAct[i]->setText(name.c_str());
            openRecentAct[i]->setVisible(true);
        } else {
            openRecentAct[i]->setVisible(false);
        }
    }
}



void
ImageViewer::reload()
{
    if (m_images.empty())
        return;
    IvImage* newimage = m_images[m_current_image];
    newimage->invalidate();
    //glwin->trigger_redraw ();
    displayCurrentImage();
}



void
ImageViewer::add_image(const std::string& filename)
{
    if (filename.empty())
        return;
    IvImage* newimage = nullptr;
    ImageSpec config;
    bool have_config = false;
    if (rawcolor()) {
        config.attribute("oiio:RawColor", 1);
        have_config = true;
    }
    if (tolerate_missing_pixels()) {
        // Ask readers that support it (OpenEXR) to fill unreadable
        // scanlines or tiles of a partially-written file with black
        // instead of failing the read. Readers without that support
        // ignore the option.
        config.attribute("oiio:missingcolor", "0");
        have_config = true;
    }
    if (have_config)
        newimage = new IvImage(filename, &config);
    else
        newimage = new IvImage(filename);
    newimage->gamma(m_default_gamma);
    m_images.push_back(newimage);
    addRecentFile(filename);
    updateRecentFilesMenu();

    if (m_images.size() == 1) {
        // If this is the first image, resize to fit it
        displayCurrentImage();
        fitWindowToImage(true, true);
    }
    updateActions();
}



void
ImageViewer::saveAs()
{
    IvImage* img = cur();
    if (!img)
        return;
    std::string name = iv_save_file(this, "Save Image", img->uname().string(),
                                    s_file_filters);
    if (name.empty())
        return;
    bool ok = img->write(name, TypeUnknown, "", image_progress_callback, this);
    if (!ok) {
        std::cerr << "Save failed: " << img->geterror() << "\n";
    }
}



void
ImageViewer::saveWindowAs()
{
    IvImage* img = cur();
    if (!img)
        return;
    ROI roi;
    glwin->get_visible_image_roi(roi);
    if (!roi.defined() || roi.width() <= 0 || roi.height() <= 0) {
        // TODO: May not be visible. This should be replaced with a GUI dialog.
        OIIO::print(stderr,
                    "Save failed: no pixels of the image are visible\n");
        return;
    }
    std::string name = iv_save_file(this, "Save Window", img->uname().string());
    if (name.empty())
        return;
    ImageBuf cropped;
    if (!ImageBufAlgo::cut(cropped, *img, roi)) {
        // TODO: May not be visible. This should be replaced with a GUI dialog.
        OIIO::print(stderr, "Crop failed: {}\n", cropped.geterror());
        return;
    }
    bool ok = cropped.write(name, TypeUnknown, "", image_progress_callback,
                            this);
    if (!ok) {
        // TODO: May not be visible. This should be replaced with a GUI dialog.
        OIIO::print(stderr, "Save failed: {}\n", cropped.geterror());
    }
}



void
ImageViewer::saveSelectionAs()
{
    IvImage* img = cur();
    if (!img)
        return;
    std::string name = iv_save_file(this, "Save Selection",
                                    img->uname().string());
    if (name.empty())
        return;
    img->write(name, TypeUnknown, "", image_progress_callback, this);
}

void
ImageViewer::moveToNewWindow()
{
#ifdef IV_USE_QT
    if (m_images.size()) {
        ImageViewer* imageViewer = new ImageViewer(m_useOCIO, m_ocioColourSpace,
                                                   m_ocioDisplay, m_ocioView);
        imageViewer->show();
        imageViewer->rawcolor(rawcolor());
        imageViewer->add_image(m_images[m_current_image]->name());
        imageViewer->current_image(0);
        imageViewer->raise();
        imageViewer->activateWindow();
    }
#else
    // Hand the image to a new iv process, and stop showing it here.
    if (IvImage* img = cur()) {
        std::vector<std::string> args { "-F" };
        if (rawcolor())
            args.emplace_back("--rawcolor");
        if (m_useOCIO)
            for (auto a : { "--image-color-space", m_ocioColourSpace.c_str(),
                            "--display", m_ocioDisplay.c_str(), "--view",
                            m_ocioView.c_str() })
                args.emplace_back(a);
        args.emplace_back(img->name());
        if (iv_launch(args))
            closeImg();
    }
#endif
}

void
ImageViewer::updateTitle()
{
    IvImage* img = cur();
    if (!img) {
        setWindowTitle(tr("iv Image Viewer (no image loaded)"));
        return;
    }
    std::string message;
    message = Strutil::fmt::format("{} - iv Image Viewer", img->name());
    setWindowTitle(message.c_str());
}



void
ImageViewer::updateStatusBar()
{
    const ImageSpec* spec = curspec();
    if (!spec) {
        statusImgInfo->setText(tr("No image loaded"));
        statusViewInfo->setText(tr(""));
        return;
    }
    std::string message;
    message = Strutil::fmt::format("({}/{}) : ", m_current_image + 1,
                                   (int)m_images.size());
    message += cur()->shortinfo();
    if (cur()->partially_loaded()) {
        message += "  [partially readable file: ";
        message += cur()->partial_error();
        message += "]";
    }
    statusImgInfo->setText(message.c_str());
    if (m_comparison_image) {
        std::string first_name  = Filesystem::filename(cur()->name());
        std::string second_name = Filesystem::filename(
            m_comparison_image->name());
        if (first_name == second_name) {
            first_name  = cur()->name();
            second_name = m_comparison_image->name();
        }
        if (m_diff_mode == DiffMode::AbsDiff)
            message += Strutil::format("  |  absdiff: {} vs {}", first_name,
                                       second_name);
        else if (m_diff_mode == DiffMode::FlipDiff)
            message += Strutil::format("  |  FLIP diff: {} vs {}", first_name,
                                       second_name);
        else {
            const char* first_side  = glwin->wipe_horizontal() ? "top" : "left";
            const char* second_side = glwin->wipe_horizontal() ? "bottom"
                                                               : "right";
            message += Strutil::format("  |  {}: {}  {}: {}  (inspecting {})",
                                       first_side, first_name, second_side,
                                       second_name, first_side);
        }
        if (m_flip_computing.load())
            message += "  |  Computing FLIP difference...";
        statusImgInfo->setText(message.c_str());
    }

    message.clear();
    switch (m_color_mode) {
    case RGBA:
        message = Strutil::fmt::format("RGBA ({}-{})", m_current_channel,
                                       m_current_channel + 3);
        break;
    case RGB:
        message = Strutil::fmt::format("RGB ({}-{})", m_current_channel,
                                       m_current_channel + 2);
        break;
    case LUMINANCE:
        message = Strutil::fmt::format("Lum ({}-{})", m_current_channel,
                                       m_current_channel + 2);
        break;
    case HEATMAP: message = "Heat ";
    case SINGLE_CHANNEL:
        if ((int)spec->channelnames.size() > m_current_channel
            && spec->channelnames[m_current_channel].size())
            message += spec->channelnames[m_current_channel];
        else if (m_color_mode == HEATMAP) {
            message += Strutil::fmt::format("{}", m_current_channel);
        } else {
            message = Strutil::fmt::format("chan {}", m_current_channel);
        }
        break;
    }
    message += Strutil::fmt::format("  {}:{}  exp {:+.1f}  gam {:.2f}",
                                    zoom() >= 1 ? zoom() : 1.0f,
                                    zoom() >= 1 ? 1.0f : 1.0f / zoom(),
                                    cur()->exposure(), cur()->gamma());
    if (cur()->nsubimages() > 1) {
        if (cur()->auto_subimage()) {
            message += Strutil::fmt::format("  subimg AUTO ({}/{})",
                                            cur()->subimage() + 1,
                                            cur()->nsubimages());
        } else {
            message += Strutil::fmt::format("  subimg {}/{}",
                                            cur()->subimage() + 1,
                                            cur()->nsubimages());
        }
    }
    if (cur()->nmiplevels() > 1) {
        message += Strutil::fmt::format("  MIP {}/{}", cur()->miplevel() + 1,
                                        cur()->nmiplevels());
    }

    statusViewInfo->setText(message.c_str());  // tr("iv status"));
}



bool
ImageViewer::loadImage(IvImage* img, int subimage, int miplevel)
{
    if (img) {
        // We need the spec available to compare the image format with
        // opengl's capabilities.
        if (!img->init_spec(img->name(), subimage, miplevel)) {
            statusImgInfo->setText(
                Strutil::format("Could not display image: {}.", img->uname())
                    .c_str());
            statusViewInfo->setText(tr(""));
            return false;
        }

        // Used to check whether we'll need to do adjustments in the
        // CPU. If true, images should be loaded as UINT8.
        bool allow_transforms = false;
        bool srgb_transform   = false;

        // By default, we try to load into OpenGL with the same format,
        TypeDesc read_format        = TypeDesc::UNKNOWN;
        const ImageSpec& image_spec = img->spec();

        if (image_spec.format.basetype == TypeDesc::DOUBLE) {
            // AFAIK, OpenGL doesn't support 64-bit floats as pixel size.
            read_format = TypeDesc::FLOAT;
        }
        if (glwin->is_glsl_capable()) {
            if (image_spec.format.basetype == TypeDesc::HALF
                && !glwin->is_half_capable()) {
                //std::cerr << "Loading HALF-FLOAT as FLOAT\n";
                read_format = TypeDesc::FLOAT;
            }
            if (is_colorspace_srgb(image_spec, false)
                && !glwin->is_srgb_capable()) {
                // If the image is in sRGB, but OpenGL can't load sRGB textures then
                // we'll need to do the transformation on the CPU after loading the
                // image. We (so far) can only do this with UINT8 images, so make
                // sure that it gets loaded in this format.
                //std::cerr << "Loading as UINT8 to do sRGB\n";
                read_format      = TypeDesc::UINT8;
                srgb_transform   = true;
                allow_transforms = true;
            }
        } else {
            //std::cerr << "Loading as UINT8\n";
            read_format      = TypeDesc::UINT8;
            allow_transforms = true;

            if (is_colorspace_srgb(image_spec, false)
                && !glwin->is_srgb_capable())
                srgb_transform = true;
        }

        // FIXME: This actually won't work since the ImageCacheFile has already
        // been created when we did the init_spec.
        // Check whether IvGL recommends generating mipmaps for this image.
        //ImageCache *imagecache = ImageCache::create (true);
        //if (glwin->is_too_big (img) && autoMipmap->isChecked ()) {
        //    imagecache->attribute ("automip", 1);
        //} else {
        //    imagecache->attribute ("automip", 0);
        //}

        // Read the image from disk or from the ImageCache if available.
        if (img->read_iv(subimage, miplevel, false, read_format,
                         image_progress_callback, this, allow_transforms)) {
            // The image was read successfully.
            // Check if we've got to do sRGB to linear (ie, when not supported
            // by OpenGL).
            // Do the first pixel transform to fill-in the secondary image
            // buffer.
            if (allow_transforms) {
                img->pixel_transform(srgb_transform, (int)current_color_mode(),
                                     current_channel());
            }
            return true;
        } else {
            statusImgInfo->setText(
                Strutil::format("Could not display image: {}.", img->uname())
                    .c_str());
            statusViewInfo->setText(tr(""));
            return false;
        }
    }
    return false;
}



bool
ImageViewer::loadCurrentImage(int subimage, int miplevel)
{
    if (m_current_image < 0 || m_current_image >= (int)m_images.size()) {
        m_current_image = 0;
    }
    return loadImage(cur(), subimage, miplevel);
}



void
ImageViewer::displayCurrentImage(bool update)
{
    if (m_current_image < 0 || m_current_image >= (int)m_images.size())
        m_current_image = 0;
    IvImage* img = cur();
    if (img) {
        if (m_comparison_image == img && m_images.size() > 1) {
            int previous       = (m_current_image + (int)m_images.size() - 1)
                                 % (int)m_images.size();
            m_comparison_image = m_images[previous];
        }
        if (!img->image_valid()) {
            bool load_result = false;

            statusViewInfo->hide();
            statusProgress->show();
            load_result = loadCurrentImage(std::max(0, img->subimage()),
                                           std::max(0, img->miplevel()));
            statusProgress->hide();
            statusViewInfo->show();

            if (load_result) {
                update = true;
            } else {
                return;
            }
        }
        if (m_comparison_image && m_comparison_image != img
            && !m_comparison_image->image_valid()) {
            if (!loadImage(m_comparison_image,
                           std::max(0, m_comparison_image->subimage()),
                           std::max(0, m_comparison_image->miplevel()))) {
                m_comparison_image = nullptr;
                m_diff_mode        = DiffMode::Off;
                m_flip_key.clear();
                IvSignalBlocker blocker(comparisonAct);
                comparisonAct->setChecked(false);
                syncDifferenceActions();
                glwin->set_diff_mode(DiffMode::Off);
                glwin->clear_flipdiff_map();
                glwin->reset_wipe();
                glwin->update();
                updateActions();
                return;
            }
        }
    } else {
        m_current_image = m_last_image = -1;
        glwin->request_redraw();
    }

    if (update) {
        glwin->update();
    }
    float z = zoom();
    if (fitImageToWindowAct->isChecked())
        z = zoom_needed_to_fit(glwin->width(), glwin->height());
    zoom(z);
    //    glwin->trigger_redraw ();

    updateTitle();
    updateStatusBar();
    if (infoWindow)
        infoWindow->update(img);

    // Recompute the FLIP error map if the displayed pair changed.
    if (m_comparison_image && m_diff_mode == DiffMode::FlipDiff)
        computeFlipDiff();

    //    fitImageToWindowAct->setEnabled(true);
    //    fullScreenAct->setEnabled(true);
    updateActions();
}



void
ImageViewer::deleteCurrentImage()
{
    IvImage* img = cur();
    if (img) {
        const char* filename = img->uname().c_str();
        if (iv_confirm(this, "",
                       Strutil::format("Are you sure you want to remove {} "
                                       "file from disk?",
                                       filename))) {
            closeImg();
            int r = remove(filename);
            if (r)
                iv_message(this, "", "Unable to delete file");
        }
    }
}



void
ImageViewer::current_image(int newimage)
{
#ifndef NDEBUG
    Timer swap_image_time;
    swap_image_time.start();
#endif
    if (m_images.empty() || newimage < 0 || newimage >= (int)m_images.size())
        m_current_image = 0;
    if (m_current_image != newimage) {
        IvImage* old_image = cur();
        if (m_comparison_image && old_image)
            m_comparison_image = old_image;
        m_last_image    = (m_current_image >= 0) ? m_current_image : newimage;
        m_current_image = newimage;
        displayCurrentImage();
    } else {
        displayCurrentImage(false);
    }
#ifndef NDEBUG
    swap_image_time.stop();
    std::cerr << "Current Image change elapsed time: " << swap_image_time()
              << " seconds \n";
#endif
}



void
ImageViewer::prevImage()
{
    if (m_images.empty())
        return;
    if (m_current_image == 0)
        current_image((int)m_images.size() - 1);
    else
        current_image(current_image() - 1);
}


void
ImageViewer::nextImage()
{
    if (m_images.empty())
        return;
    if (m_current_image >= (int)m_images.size() - 1)
        current_image(0);
    else
        current_image(current_image() + 1);
}



void
ImageViewer::toggleImage()
{
    if (m_comparison_image) {
        auto found = std::find(m_images.begin(), m_images.end(),
                               m_comparison_image);
        if (found != m_images.end())
            current_image(int(found - m_images.begin()));
    } else {
        current_image(m_last_image);
    }
}

void
ImageViewer::toggleComparison(bool enabled)
{
    if (enabled) {
        if (!glwin->is_glsl_capable() || m_images.size() < 2 || !cur()) {
            // FIXME: Output to stderr is not ideal. This should be a dialog box.
            std::cerr << "iv: wipe comparison needs GLSL and at least two "
                         "images\n";
            IvSignalBlocker blocker(comparisonAct);
            comparisonAct->setChecked(false);
            return;
        }
        // The wipe and the difference view are mutually exclusive; enabling
        // the wipe turns the difference view off.
        m_diff_mode = DiffMode::Off;
        syncDifferenceActions();
        m_comparison_image = pickComparisonImage();
        glwin->set_diff_mode(DiffMode::Off);
        glwin->clear_flipdiff_map();
        glwin->reset_wipe();
        displayCurrentImage();
    } else {
        m_comparison_image = nullptr;
        m_flip_key.clear();
        glwin->clear_flipdiff_map();
        if (comparisonAct && comparisonAct->isChecked()) {
            IvSignalBlocker blocker(comparisonAct);
            comparisonAct->setChecked(false);
        }
        displayCurrentImage();
    }
}



void
ImageViewer::startComparison(DiffMode mode)
{
    if (m_images.size() < 2) {
        // FIXME: Output to stderr is not ideal. This should be a dialog box.
        std::cerr << "iv: --compare needs at least two images\n";
        return;
    }
    if (!glwin->is_glsl_capable()) {
        // FIXME: Output to stderr is not ideal. This should be a dialog box.
        std::cerr << "iv: comparison modes require GLSL, ignoring --compare\n";
        return;
    }
    if (mode == DiffMode::Off)
        comparisonAct->setChecked(true);  // triggers toggleComparison(true)
    else
        setDiffMode(mode);
}



void
ImageViewer::setDiffMode(DiffMode mode)
{
    if (mode == m_diff_mode && (mode == DiffMode::Off || m_comparison_image))
        return;
    if (mode != DiffMode::Off) {
        if (!glwin->is_glsl_capable() || m_images.size() < 2 || !cur()) {
            // FIXME: Output to stderr is not ideal. This should be a dialog box.
            std::cerr << "iv: difference view needs GLSL and at least two "
                         "images\n";
            syncDifferenceActions();  // snap the radio back to the real state
            return;
        }
        // The wipe and the difference view are mutually exclusive; enabling
        // the difference view turns the wipe off.
        if (comparisonAct && comparisonAct->isChecked()) {
            IvSignalBlocker blocker(comparisonAct);
            comparisonAct->setChecked(false);
        }
    }
    m_diff_mode = mode;
    syncDifferenceActions();
    glwin->set_diff_mode(mode);
    if (mode == DiffMode::Off) {
        // Leaving the difference view: no wipe is active (mutual exclusion),
        // so the pair goes away and the plain image returns.
        m_comparison_image = nullptr;
        m_flip_key.clear();
        glwin->clear_flipdiff_map();
    } else {
        m_comparison_image = pickComparisonImage();
        glwin->clear_flipdiff_map();
    }
    displayCurrentImage();
}



void
ImageViewer::syncDifferenceActions()
{
    if (!differenceOffAct)
        return;
    IvSignalBlocker b1(differenceOffAct);
    IvSignalBlocker b2(differenceAbsDiffAct);
    IvSignalBlocker b3(differenceFlipDiffAct);
    differenceOffAct->setChecked(m_diff_mode == DiffMode::Off);
    differenceAbsDiffAct->setChecked(m_diff_mode == DiffMode::AbsDiff);
    differenceFlipDiffAct->setChecked(m_diff_mode == DiffMode::FlipDiff);
}



IvImage*
ImageViewer::pickComparisonImage() const
{
    // Compare against the image viewed most recently before this one (the
    // same pairing the wipe uses); fall back to the previous image in the
    // list. Callers guarantee at least two images and a current image.
    int index = m_last_image;
    if (index < 0 || index >= (int)m_images.size() || index == m_current_image)
        index = (m_current_image + (int)m_images.size() - 1)
                % (int)m_images.size();
    return m_images[index];
}



void
ImageViewer::differenceModeChanged()
{
    // Derive the new mode from whichever action is now checked.
    DiffMode mode = DiffMode::Off;
    if (differenceAbsDiffAct->isChecked())
        mode = DiffMode::AbsDiff;
    else if (differenceFlipDiffAct->isChecked())
        mode = DiffMode::FlipDiff;
    if (mode != m_diff_mode)
        setDiffMode(mode);
}



void
ImageViewer::cycleDifference()
{
    DiffMode mode = DiffMode::Off;
    if (m_diff_mode == DiffMode::Off)
        mode = DiffMode::AbsDiff;
    else if (m_diff_mode == DiffMode::AbsDiff)
        mode = DiffMode::FlipDiff;
    setDiffMode(mode);
}



void
ImageViewer::flipColormapToggled(bool checked)
{
    m_flip_colormap = checked;
    if (m_comparison_image && m_diff_mode == DiffMode::FlipDiff) {
        glwin->clear_flipdiff_map();
        glwin->request_redraw();
        computeFlipDiff();
    }
}



void
ImageViewer::computeFlipDiff()
{
    if (!m_comparison_image || m_diff_mode != DiffMode::FlipDiff)
        return;
    IvImage* img = cur();
    if (!img || !img->image_valid() || !m_comparison_image->image_valid())
        return;

    // Signature of the inputs, to skip redundant recomputes.
    auto key = Strutil::format("{}|{}|{}|{}|{}", img->name(),
                               m_comparison_image->name(), img->subimage(),
                               img->miplevel(), m_flip_colormap);
    if (m_flip_computing.load()) {
        m_flip_pending = true;  // worker running; it will be re-kicked
        return;
    }
    if (key == m_flip_key)
        return;
    m_flip_key = key;

    // ImageBuf copies are cheap (they share the underlying buffer), so
    // it's safe to hand these to the worker thread.
    ImageBuf a(*img);
    ImageBuf b(*m_comparison_image);
    bool colormap = m_flip_colormap;
    // Re-assigning a joinable std::thread would terminate(), so join the
    // finished worker first (m_flip_computing == false implies it has
    // delivered its result and is on its way out).
    if (m_flip_thread.joinable())
        m_flip_thread.join();
    m_flip_computing.store(true);
    // FLIP can take a while on large images; make it obvious that
    // something is happening. The indicator goes through
    // updateStatusBar() rather than QStatusBar::showMessage(), whose
    // temporary label overprints the permanent status labels.
    IvApplication::setOverrideCursor(Iv::BusyCursor);
    updateStatusBar();
    m_flip_thread = std::thread([this, a, b, colormap]() {
        ImageBuf err;
        if (a.spec().width == b.spec().width
            && a.spec().height == b.spec().height)
            err = ImageBufAlgo::FLIP_diff(a, b);
        if (colormap && err.initialized())
            err = ImageBufAlgo::color_map(err, 0, "inferno");
        // Deliver on the GUI thread.
        iv_post(this, [this, err]() { flipDiffDone(err); });
    });
}



void
ImageViewer::flipDiffDone(ImageBuf errmap)
{
    m_flip_computing.store(false);
    if (IvApplication::overrideCursor())
        IvApplication::restoreOverrideCursor();
    if (errmap.initialized() && m_diff_mode == DiffMode::FlipDiff
        && m_comparison_image)
        glwin->set_flipdiff_map(errmap);
    // IvGL::update only refreshes textures, not a repaint.
    glwin->request_redraw();
    if (m_flip_pending.exchange(false))
        computeFlipDiff();
    else
        updateStatusBar();
}



void
ImageViewer::toggleWindowGuides()
{
    glwin->request_redraw();
}



void
ImageViewer::exposureMinusOneTenthStop()
{
    if (m_images.empty())
        return;
    IvImage* img = m_images[m_current_image];
    img->exposure(img->exposure() - 0.1);
    if (!glwin->is_glsl_capable()) {
        bool srgb_transform = (!glwin->is_srgb_capable()
                               && is_colorspace_srgb(img->spec(), false));
        img->pixel_transform(srgb_transform, (int)current_color_mode(),
                             current_channel());
        displayCurrentImage();
    } else {
        displayCurrentImage(false);
    }
}


void
ImageViewer::exposureMinusOneHalfStop()
{
    if (m_images.empty())
        return;
    IvImage* img = m_images[m_current_image];
    img->exposure(img->exposure() - 0.5);
    if (!glwin->is_glsl_capable()) {
        bool srgb_transform = (!glwin->is_srgb_capable()
                               && is_colorspace_srgb(img->spec(), false));
        img->pixel_transform(srgb_transform, (int)current_color_mode(),
                             current_channel());
        displayCurrentImage();
    } else {
        displayCurrentImage(false);
    }
}


void
ImageViewer::exposurePlusOneTenthStop()
{
    if (m_images.empty())
        return;
    IvImage* img = m_images[m_current_image];
    img->exposure(img->exposure() + 0.1);
    if (!glwin->is_glsl_capable()) {
        bool srgb_transform = (!glwin->is_srgb_capable()
                               && is_colorspace_srgb(img->spec(), false));
        img->pixel_transform(srgb_transform, (int)current_color_mode(),
                             current_channel());
        displayCurrentImage();
    } else {
        displayCurrentImage(false);
    }
}


void
ImageViewer::exposurePlusOneHalfStop()
{
    if (m_images.empty())
        return;
    IvImage* img = m_images[m_current_image];
    img->exposure(img->exposure() + 0.5);
    if (!glwin->is_glsl_capable()) {
        bool srgb_transform = (!glwin->is_srgb_capable()
                               && is_colorspace_srgb(img->spec(), false));
        img->pixel_transform(srgb_transform, (int)current_color_mode(),
                             current_channel());
        displayCurrentImage();
    } else {
        displayCurrentImage(false);
    }
}



void
ImageViewer::gammaMinus()
{
    if (m_images.empty())
        return;
    IvImage* img = m_images[m_current_image];
    img->gamma(img->gamma() - 0.05);
    if (!glwin->is_glsl_capable()) {
        bool srgb_transform = (!glwin->is_srgb_capable()
                               && is_colorspace_srgb(img->spec(), false));
        img->pixel_transform(srgb_transform, (int)current_color_mode(),
                             current_channel());
        displayCurrentImage();
    } else {
        displayCurrentImage(false);
    }
}


void
ImageViewer::gammaPlus()
{
    if (m_images.empty())
        return;
    IvImage* img = m_images[m_current_image];
    img->gamma(img->gamma() + 0.05);
    if (!glwin->is_glsl_capable()) {
        bool srgb_transform = (!glwin->is_srgb_capable()
                               && is_colorspace_srgb(img->spec(), false));
        img->pixel_transform(srgb_transform, (int)current_color_mode(),
                             current_channel());
        displayCurrentImage();
    } else {
        displayCurrentImage(false);
    }
}



void
ImageViewer::slide(long /*t*/, bool b)
{
    slideLoopAct->setChecked(b == true);
    slideNoLoopAct->setChecked(b == false);
}



void
ImageViewer::viewChannel(int c, COLOR_MODE colormode)
{
#ifndef NDEBUG
    Timer change_channel_time;
    change_channel_time.start();
#endif
    if (m_current_channel != c || colormode != m_color_mode) {
        bool update = true;
        if (!glwin->is_glsl_capable()) {
            IvImage* img = cur();
            if (img) {
                bool srgb_transform = (!glwin->is_srgb_capable()
                                       && is_colorspace_srgb(img->spec(),
                                                             false));
                img->pixel_transform(srgb_transform, (int)colormode, c);
            }
        } else {
            // FIXME: There are even more chances to avoid updating the textures
            // if we can keep trac of which channels are in the texture.
            if (m_current_channel == c) {
                if (m_color_mode == SINGLE_CHANNEL || m_color_mode == HEATMAP) {
                    if (colormode == HEATMAP || colormode == SINGLE_CHANNEL)
                        update = false;
                } else if (m_color_mode == RGB || m_color_mode == LUMINANCE) {
                    if (colormode == RGB || colormode == LUMINANCE)
                        update = false;
                }
            }
        }
        m_current_channel = c;
        m_color_mode      = colormode;
        displayCurrentImage(update);

        viewChannelFullAct->setChecked(c == 0 && m_color_mode == RGBA);
        viewChannelRedAct->setChecked(c == 0 && m_color_mode == SINGLE_CHANNEL);
        viewChannelGreenAct->setChecked(c == 1
                                        && m_color_mode == SINGLE_CHANNEL);
        viewChannelBlueAct->setChecked(c == 2
                                       && m_color_mode == SINGLE_CHANNEL);
        viewChannelAlphaAct->setChecked(c == 3
                                        && m_color_mode == SINGLE_CHANNEL);
        viewColorLumAct->setChecked(m_color_mode == LUMINANCE);
        viewColorRGBAAct->setChecked(m_color_mode == RGBA);
        viewColorRGBAct->setChecked(m_color_mode == RGB);
        viewColor1ChAct->setChecked(m_color_mode == SINGLE_CHANNEL);
        viewColorHeatmapAct->setChecked(m_color_mode == HEATMAP);
    }
#ifndef NDEBUG
    change_channel_time.stop();
    std::cerr << "Change channel elapsed time: " << change_channel_time()
              << " seconds \n";
#endif
}


void
ImageViewer::slideImages()
{
    if (m_images.empty())
        return;
    if (m_current_image >= (int)m_images.size() - 1) {
        if (slide_loop == true)
            current_image(0);
        else {
            slideTimer->stop();
            disconnect(slideTimer, 0, 0, 0);
        }
    } else
        current_image(current_image() + 1);
}


void
ImageViewer::slideShow()
{
    fullScreenToggle();
    connect(slideTimer, &IvTimer::timeout, this, &ImageViewer::slideImages);
    slideTimer->start(slideDuration_ms);
    updateActions();
}



void
ImageViewer::slideLoop()
{
    slide_loop = true;
    slide(slideDuration_ms, slide_loop);
}


void
ImageViewer::slideNoLoop()
{
    slide_loop = false;
    slide(slideDuration_ms, slide_loop);
}


void
ImageViewer::setSlideShowDuration(int seconds)
{
    slideDuration_ms = seconds * 1000;
}



static bool
compName(IvImage* first, IvImage* second)
{
    std::string firstFile  = Filesystem::filename(first->name());
    std::string secondFile = Filesystem::filename(second->name());
    return (firstFile.compare(secondFile) < 0);
}



void
ImageViewer::sortByName()
{
    int numImg = m_images.size();
    if (numImg < 2)
        return;
    std::sort(m_images.begin(), m_images.end(), &compName);
    current_image(0);
    displayCurrentImage();
    // updateActions();
}



static bool
compPath(IvImage* first, IvImage* second)
{
    std::string firstFile  = first->name();
    std::string secondFile = second->name();
    return (firstFile.compare(secondFile) < 0);
}



void
ImageViewer::sortByPath()
{
    int numImg = m_images.size();
    if (numImg < 2)
        return;
    std::sort(m_images.begin(), m_images.end(), &compPath);
    current_image(0);
    displayCurrentImage();
    // updateActions();
}



static bool
DateTime_to_time_t(string_view datetime, time_t& timet)
{
    int year, month, day, hour, min, sec;
    if (!Strutil::scan_datetime(datetime, year, month, day, hour, min, sec))
        return false;
    // print("{}:{}:{} {}:{}:{}\n", year, month, day, hour, min, sec);
    struct tm tmtime;
    time_t now = time(nullptr);
    Sysutil::get_local_time(&now, &tmtime);  // fill in defaults
    tmtime.tm_sec  = sec;
    tmtime.tm_min  = min;
    tmtime.tm_hour = hour;
    tmtime.tm_mday = day;
    tmtime.tm_mon  = month - 1;
    tmtime.tm_year = year - 1900;
    timet          = mktime(&tmtime);
    return true;
}



static bool
compImageDate(IvImage* first, IvImage* second)
{
    std::time_t firstFile  = time(NULL);
    std::time_t secondFile = time(NULL);
    double diff;
    std::string metadatatime = first->spec().get_string_attribute("DateTime");
    if (metadatatime.empty()) {
        if (first->init_spec(first->name(), 0, 0)) {
            metadatatime = first->spec().get_string_attribute("DateTime");
            if (metadatatime.empty()) {
                if (!Filesystem::exists(first->name()))
                    return false;
                firstFile = Filesystem::last_write_time(first->name());
            }
        } else
            return false;
    }
    DateTime_to_time_t(metadatatime.c_str(), firstFile);
    metadatatime = second->spec().get_string_attribute("DateTime");
    if (metadatatime.empty()) {
        if (second->init_spec(second->name(), 0, 0)) {
            metadatatime = second->spec().get_string_attribute("DateTime");
            if (metadatatime.empty()) {
                if (!Filesystem::exists(second->name()))
                    return true;
                secondFile = Filesystem::last_write_time(second->name());
            }
        } else
            return true;
    }
    DateTime_to_time_t(metadatatime.c_str(), secondFile);
    diff = difftime(firstFile, secondFile);
    if (diff == 0)
        return compName(first, second);
    return (diff < 0);
}



void
ImageViewer::sortByImageDate()
{
    int numImg = m_images.size();
    if (numImg < 2)
        return;
    std::sort(m_images.begin(), m_images.end(), &compImageDate);
    current_image(0);
    displayCurrentImage();
    // updateActions();
}



static bool
compFileDate(IvImage* first, IvImage* second)
{
    std::time_t firstFile, secondFile;
    double diff;
    if (!Filesystem::exists(first->name()))
        return false;
    firstFile = Filesystem::last_write_time(first->name());
    if (!Filesystem::exists(second->name()))
        return true;
    secondFile = Filesystem::last_write_time(second->name());
    diff       = difftime(firstFile, secondFile);
    if (diff == 0)
        return compName(first, second);
    return (diff < 0);
}



void
ImageViewer::sortByFileDate()
{
    int numImg = m_images.size();
    if (numImg < 2)
        return;
    std::sort(m_images.begin(), m_images.end(), &compFileDate);
    current_image(0);
    displayCurrentImage();
    // updateActions();
}



void
ImageViewer::sortReverse()
{
    int numImg = m_images.size();
    if (numImg < 2)
        return;
    std::reverse(m_images.begin(), m_images.end());
    current_image(0);
    displayCurrentImage();
    // updateActions();
}



void
ImageViewer::viewChannelFull()
{
    viewChannel(0, RGBA);
}


void
ImageViewer::viewChannelRed()
{
    viewChannel(0, SINGLE_CHANNEL);
}


void
ImageViewer::viewChannelGreen()
{
    viewChannel(1, SINGLE_CHANNEL);
}


void
ImageViewer::viewChannelBlue()
{
    viewChannel(2, SINGLE_CHANNEL);
}


void
ImageViewer::viewChannelAlpha()
{
    viewChannel(3, SINGLE_CHANNEL);
}


void
ImageViewer::viewChannelLuminance()
{
    viewChannel(m_current_channel, LUMINANCE);
}


void
ImageViewer::viewColorRGBA()
{
    viewChannel(m_current_channel, RGBA);
}


void
ImageViewer::viewColorRGB()
{
    viewChannel(m_current_channel, RGB);
}


void
ImageViewer::viewColor1Ch()
{
    viewChannel(m_current_channel, SINGLE_CHANNEL);
}


void
ImageViewer::viewColorHeatmap()
{
    viewChannel(m_current_channel, HEATMAP);
}


void
ImageViewer::viewChannelPrev()
{
    if (glwin->is_glsl_capable()) {
        if (m_current_channel > 0)
            viewChannel(m_current_channel - 1, m_color_mode);
    } else {
        // Simulate old behavior.
        if (m_color_mode == RGBA || m_color_mode == RGB) {
            viewChannel(m_current_channel, LUMINANCE);
        } else if (m_color_mode == SINGLE_CHANNEL) {
            if (m_current_channel == 0)
                viewChannelFull();
            else
                viewChannel(m_current_channel - 1, SINGLE_CHANNEL);
        }
    }
}


void
ImageViewer::viewChannelNext()
{
    if (glwin->is_glsl_capable()) {
        viewChannel(m_current_channel + 1, m_color_mode);
    } else {
        // Simulate old behavior.
        if (m_color_mode == LUMINANCE) {
            viewChannelFull();
        } else if (m_color_mode == RGBA || m_color_mode == RGB) {
            viewChannelRed();
        } else if (m_color_mode == SINGLE_CHANNEL) {
            viewChannel(m_current_channel + 1, SINGLE_CHANNEL);
        }
    }
}



void
ImageViewer::viewSubimagePrev()
{
    IvImage* img = cur();
    if (!img)
        return;
    bool ok = false;
    if (img->miplevel() > 0) {
        ok = loadCurrentImage(img->subimage(), img->miplevel() - 1);
    } else if (img->subimage() > 0) {
        ok = loadCurrentImage(img->subimage() - 1);
    } else if (img->nsubimages() > 0) {
        img->auto_subimage(true);
        ok = loadCurrentImage(0);
    }
    if (ok) {
        if (fitImageToWindowAct->isChecked())
            fitImageToWindow();
        displayCurrentImage();
    }
}


void
ImageViewer::viewSubimageNext()
{
    IvImage* img = cur();
    if (!img)
        return;
    bool ok = false;
    if (img->auto_subimage()) {
        img->auto_subimage(false);
        ok = loadCurrentImage(0);
    } else if (img->miplevel() < img->nmiplevels() - 1) {
        ok = loadCurrentImage(img->subimage(), img->miplevel() + 1);
    } else if (img->subimage() < img->nsubimages() - 1) {
        ok = loadCurrentImage(img->subimage() + 1);
    }
    if (ok) {
        if (fitImageToWindowAct->isChecked())
            fitImageToWindow();
        displayCurrentImage();
    }
}



void
ImageViewer::keyPressEvent(IvKeyEvent* event)
{
    switch (event->key()) {
    case Iv::Key_Left:
    case Iv::Key_Up:
    case Iv::Key_PageUp: prevImage(); return;  //break;
    case Iv::Key_Right:
        // print(stderr, "Modifier is {0:d} 0x(0:x}\n", event->modifiers());
        // if (event->modifiers() & Iv::ShiftModifier)
        //     print(stderr, "hey, ctrl right\n");

    case Iv::Key_Down:
    case Iv::Key_PageDown: nextImage(); return;  //break;
    case Iv::Key_Escape:
        if (m_comparison_image) {
            toggleComparison(false);
            return;
        }
        if (m_fullscreen)
            fullScreenToggle();
        return;
    case Iv::Key_Minus:
    case Iv::Key_Underscore: zoomOut(); break;
    case Iv::Key_Plus:
    case Iv::Key_Equal: zoomIn(); break;
    default:
        // print(stderr, "ImageViewer key {:d}\n", event->key());
        IvMainWindow::keyPressEvent(event);
    }
}



void
ImageViewer::resizeEvent(IvResizeEvent* event)
{
    if (fitImageToWindowAct->isChecked())
        fitImageToWindow();
    IvMainWindow::resizeEvent(event);
}



void
ImageViewer::closeImg()
{
    if (m_images.empty())
        return;
    delete m_images[m_current_image];
    m_images[m_current_image] = NULL;
    m_images.erase(m_images.begin() + m_current_image);

    // Update image indices
    // This should be done for all image indices we may be storing
    if (m_last_image == m_current_image) {
        if (!m_images.empty() && m_last_image > 0)
            m_last_image = 0;
        else
            m_last_image = -1;
    }
    if (m_last_image > m_current_image)
        m_last_image--;

    m_current_image = m_current_image < (int)m_images.size() ? m_current_image
                                                             : 0;
    displayCurrentImage();
}



void
ImageViewer::zoomIn(bool smooth)
{
    IvImage* img = cur();
    if (!img)
        return;

    float current_zoom = zoom();
    if (current_zoom >= 64)
        return;

    float newzoom = ceil2f(current_zoom);

    this->zoomToCursor(newzoom, smooth);
}


void
ImageViewer::zoomOut(bool smooth)
{
    IvImage* img = cur();
    if (!img)
        return;

    float current_zoom = zoom();
    if (current_zoom <= 1.0f / 64)
        return;

    float newzoom = floor2f(current_zoom);

    this->zoomToCursor(newzoom, smooth);
}


void
ImageViewer::zoomToCursor(float newzoom, bool smooth)
{
    float oldzoom = zoom();
    float xc, yc;  // Center view position
    glwin->get_center(xc, yc);
    int xm, ym;  // Mouse position
    glwin->get_focus_image_pixel(xm, ym);
    float xoffset = xc - xm;
    float yoffset = yc - ym;

    float maxzoomratio = std::max(oldzoom / newzoom, newzoom / oldzoom);
    int nsteps = smooth ? (int)OIIO::clamp(20 * (maxzoomratio - 1), 2.0f, 10.0f)
                        : 1;
    for (int i = 1; i <= nsteps; ++i) {
        float a         = (float)i / (float)nsteps;  // Interpolation amount
        float z         = OIIO::lerp(oldzoom, newzoom, a);
        float zoomratio = z / oldzoom;
        view(xm + xoffset / zoomratio, ym + yoffset / zoomratio, z, false);
        if (i != nsteps) {
            IvApplication::processEvents();
            Sysutil::usleep(1000000 / 4 / nsteps);
        }
    }

    fitImageToWindowAct->setChecked(false);
}


void
ImageViewer::normalSize()
{
    IvImage* img = cur();
    if (!img)
        return;
    fitImageToWindowAct->setChecked(false);
    float xcenter = img->oriented_full_x() + 0.5 * img->oriented_full_width();
    float ycenter = img->oriented_full_y() + 0.5 * img->oriented_full_height();
    view(xcenter, ycenter, 1.0, true);
    fitWindowToImage(false);
}



float
ImageViewer::zoom_needed_to_fit(int w, int h)
{
    IvImage* img = cur();
    if (!img)
        return 1;
    float zw = (float)w / img->oriented_width();
    float zh = (float)h / img->oriented_height();
    return std::min(zw, zh);
}



void
ImageViewer::fitImageToWindow()
{
    IvImage* img = cur();
    if (!img)
        return;
    fitImageToWindowAct->setChecked(true);
    zoom(zoom_needed_to_fit(glwin->width(), glwin->height()));
}



void
ImageViewer::fitWindowToImage(bool zoomok, bool minsize)
{
    IvImage* img = cur();
    // Don't resize when there's no image or the image hasn't been opened yet
    // (or we failed to open it).
    if (!img || !img->image_valid())
        return;
    // FIXME -- figure out a way to make it exactly right, even for the
    // main window border, etc.
#ifdef __APPLE__
    int extraw = 0;  //12; // width() - minimumWidth();
    int extrah = statusBar()->height()
                 + 0;  //40; // height() - minimumHeight();
#else
    int extraw = 4;  //12; // width() - minimumWidth();
    int extrah = statusBar()->height()
                 + 4;  //40; // height() - minimumHeight();
#endif
    //    std::cerr << "extra wh = " << extraw << ' ' << extrah << '\n';

    float z = zoom();
    int w   = (int)(img->oriented_full_width() * z) + extraw;
    int h   = (int)(img->oriented_full_height() * z) + extrah;
    if (minsize) {
        if (w < m_default_width) {
            w = m_default_width;
        }
        if (h < m_default_height) {
            h = m_default_height;
        }
    }

    if (!m_fullscreen) {
        int availx, availy, availw, availh;
        iv_available_screen(this, availx, availy, availw, availh);
        int availwidth  = availw - extraw - 20;
        int availheight = availh - extrah - menuBar()->height() - 20;
        if (w > availwidth || h > availheight) {
            w = std::min(w, availwidth);
            h = std::min(h, availheight);
            if (zoomok) {
                z = zoom_needed_to_fit(w, h);
                w = (int)(img->oriented_full_width() * z) + extraw;
                h = (int)(img->oriented_full_height() * z) + extrah;
                // std::cerr << "must rezoom to " << z << " to fit\n";
            }
            // std::cerr << "New window geom " << w << "x" << h << "\n";
            int posx = x(), posy = y();
            if (posx + w > availwidth || posy + h > availheight) {
                if (posx + w > availwidth)
                    posx = std::max(0, availwidth - w) + availx;
                if (posy + h > availheight)
                    posy = std::max(0, availheight - h) + availy;
                // std::cerr << "New position " << posx << ' ' << posy << "\n";
                move(posx, posy);
            }
        }
    }

    float midx = img->oriented_full_x() + 0.5 * img->oriented_full_width();
    float midy = img->oriented_full_y() + 0.5 * img->oriented_full_height();
    view(midx, midy, z, false, false);
    resize(w, h);  // Resize will trigger a repaint.

#if 0
    bool fit = fitWindowToImageAct->isChecked();
    if (!fit) {
        normalSize();
    }
#endif
    updateActions();
}



void
ImageViewer::fullScreenToggle()
{
    if (m_fullscreen) {
        menuBar()->show();
        statusBar()->show();
        showNormal();
        m_fullscreen = false;
        slideTimer->stop();
        disconnect(slideTimer, 0, 0, 0);
    } else {
        menuBar()->hide();
        statusBar()->hide();
        showFullScreen();
        m_fullscreen = true;
        fitImageToWindow();
    }
}



static std::string
format_list(OIIO::string_view attr)
{
    auto formats = OIIO::Strutil::splitsv(OIIO::get_string_attribute(attr),
                                          ",");
    std::sort(formats.begin(), formats.end());
    return OIIO::Strutil::join(formats, ", ");
}



void
ImageViewer::showImageFormats()
{
    std::string text;
    text += "Input formats:\n";
    text += format_list("input_format_list");
    text += "\n\n";
    text += "Output formats:\n";
    text += format_list("output_format_list");
    text += "\n\n";
    text += "Extensions by format:\n";
    text += OIIO::get_string_attribute("extension_list");
    text += "\n";
    iv_text_dialog(this, "Supported Image Formats", text);
}



void
ImageViewer::about()
{
#ifdef IV_USE_QT
    QMessageBox::about(
        this, tr("About iv"),
        tr("<p><b>iv</b> is the image viewer for OpenImageIO.</p>"
           "<p>(c) Copyright Contributors to the OpenImageIO project.</p>"
           "<p>See <a href='https://openimageio.org'>https://openimageio.org</a> for details.</p>"));
#else
    iv_message(this, "About iv",
               "iv is the image viewer for OpenImageIO.\n\n"
               "(c) Copyright Contributors to the OpenImageIO project.\n\n"
               "See https://openimageio.org for details.");
#endif
}


void
ImageViewer::updateActions()
{
    //    zoomInAct->setEnabled(!fitImageToWindowAct->isChecked());
    //    zoomOutAct->setEnabled(!fitImageToWindowAct->isChecked());
    //    normalSizeAct->setEnabled(!fitImageToWindowAct->isChecked());
    const bool can_compare = m_images.size() >= 2 && glwin->is_glsl_capable();
    if (comparisonAct) {
        comparisonAct->setEnabled(can_compare);
        IvSignalBlocker blocker(comparisonAct);
        comparisonAct->setChecked(can_compare && m_comparison_image
                                  && m_diff_mode == DiffMode::Off);
    }
    if (differenceMenu)
        differenceMenu->setEnabled(can_compare);
    if (!can_compare && m_comparison_image) {
        m_comparison_image = nullptr;
        m_diff_mode        = DiffMode::Off;
        m_flip_key.clear();
        glwin->set_diff_mode(DiffMode::Off);
        glwin->clear_flipdiff_map();
        syncDifferenceActions();
        glwin->reset_wipe();
        glwin->update();
    }
}



static inline void
calc_subimage_from_zoom(const IvImage* img, int& subimage, float& zoom,
                        float& xcenter, float& ycenter)
{
    int rel_subimage = std::trunc(std::log2(1.0f / zoom));
    subimage         = clamp<int>(img->subimage() + rel_subimage, 0,
                                  img->nsubimages() - 1);
    if (!(img->subimage() == 0 && zoom > 1)
        && !(img->subimage() == img->nsubimages() - 1 && zoom < 1)) {
        float pow_zoom = powf(2.0f, (float)rel_subimage);
        zoom *= pow_zoom;
        xcenter /= pow_zoom;
        ycenter /= pow_zoom;
    }
}



void
ImageViewer::view(float xcenter, float ycenter, float newzoom, bool smooth,
                  bool redraw)
{
    IvImage* img = cur();
    if (!img)
        return;

    float oldzoom = m_zoom;
    float oldxcenter, oldycenter;
    glwin->get_center(oldxcenter, oldycenter);
    float zoomratio = std::max(oldzoom / newzoom, newzoom / oldzoom);
    int nsteps      = (int)OIIO::clamp(20 * (zoomratio - 1), 2.0f, 10.0f);
    if (!smooth || !redraw)
        nsteps = 1;
    for (int i = 1; i <= nsteps; ++i) {
        float a  = (float)i / (float)nsteps;  // Interpolation amount
        float xc = OIIO::lerp(oldxcenter, xcenter, a);
        float yc = OIIO::lerp(oldycenter, ycenter, a);
        m_zoom   = OIIO::lerp(oldzoom, newzoom, a);

        glwin->view(xc, yc, m_zoom, redraw);  // Triggers redraw automatically
        if (i != nsteps) {
            IvApplication::processEvents();
            Sysutil::usleep(1000000 / 4 / nsteps);
        }
    }

    if (img->auto_subimage() && !m_comparison_image) {
        int subimage = 0;
        calc_subimage_from_zoom(img, subimage, m_zoom, xcenter, ycenter);
        if (subimage != img->subimage()) {
            //std::cerr << "Changing to subimage " << subimage;
            //std::cerr << " With zoom: " << m_zoom << '\n';
            loadCurrentImage(subimage);
            glwin->update();
            glwin->view(xcenter, ycenter, m_zoom, redraw);
        }
    }

    //    zoomInAct->setEnabled (zoom() < 64.0);
    //    zoomOutAct->setEnabled (zoom() > 1.0/64);

    updateStatusBar();
}



void
ImageViewer::zoom(float newzoom, bool smooth)
{
    float xcenter, ycenter;
    glwin->get_center(xcenter, ycenter);
    view(xcenter, ycenter, newzoom, smooth);
}



void
ImageViewer::showInfoWindow()
{
    if (!infoWindow) {
        infoWindow = new IvInfoWindow(*this, true);
#ifdef IV_USE_QT
        infoWindow->setPalette(m_palette);
#endif
    }
    infoWindow->update(cur());
    if (infoWindow->isHidden())
        infoWindow->show();
    else
        infoWindow->hide();
}



void
ImageViewer::showPixelviewWindow()
{
    glwin->request_redraw();
}



void
ImageViewer::editPreferences()
{
    if (!preferenceWindow) {
        preferenceWindow = new IvPreferenceWindow(*this);
#ifdef IV_USE_QT
        preferenceWindow->setPalette(m_palette);
#endif
    }
    preferenceWindow->show();
}



void
ImageViewer::toggleAreaSample()
{
    m_areaSampleMode = !m_areaSampleMode;
    if (m_areaSampleMode) {
        setCursor(Iv::CrossCursor);
    } else {
        unsetCursor();
    }
    // if (m_areaSampleMode == false){
    //     updateStatusBar();
    // }
    glwin->request_redraw();
}



bool
ImageViewer::areaSampleMode() const
{
    return m_areaSampleMode;
}


void
ImageViewer::rotateLeft()
{
    IvImage* img = cur();
    if (!img)
        return;

    ImageSpec* spec = curspecmod();

    int curr_orientation = spec->get_int_attribute("Orientation", 1);

    if (curr_orientation >= 1 && curr_orientation <= 8) {
        static int next_orientation[] = { 0, 8, 5, 6, 7, 4, 1, 2, 3 };
        curr_orientation              = next_orientation[curr_orientation];
        spec->attribute("Orientation", curr_orientation);
    }
    displayCurrentImage();
}


void
ImageViewer::rotateRight()
{
    IvImage* img = cur();
    if (!img)
        return;

    ImageSpec* spec      = curspecmod();
    int curr_orientation = spec->get_int_attribute("Orientation", 1);

    if (curr_orientation >= 1 && curr_orientation <= 8) {
        static int next_orientation[] = { 0, 6, 7, 8, 5, 2, 3, 4, 1 };
        curr_orientation              = next_orientation[curr_orientation];
        spec->attribute("Orientation", curr_orientation);
    }
    displayCurrentImage();
}


void
ImageViewer::flipHorizontal()
{
    IvImage* img = cur();
    if (!img)
        return;

    ImageSpec* spec = curspecmod();

    int curr_orientation = spec->get_int_attribute("Orientation", 1);

    if (curr_orientation >= 1 && curr_orientation <= 8) {
        static int next_orientation[] = { 0, 2, 1, 4, 3, 6, 5, 8, 7 };
        curr_orientation              = next_orientation[curr_orientation];
        spec->attribute("Orientation", curr_orientation);
    }
    displayCurrentImage();
}


void
ImageViewer::flipVertical()
{
    IvImage* img = cur();
    if (!img)
        return;

    ImageSpec* spec = curspecmod();

    int curr_orientation = spec->get_int_attribute("Orientation", 1);

    if (curr_orientation >= 1 && curr_orientation <= 8) {
        static int next_orientation[] = { 0, 4, 3, 2, 1, 8, 7, 6, 5 };
        curr_orientation              = next_orientation[curr_orientation];
        spec->attribute("Orientation", curr_orientation);
    }
    displayCurrentImage();
}
