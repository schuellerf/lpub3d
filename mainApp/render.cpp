 
/****************************************************************************
**
** Copyright (C) 2007-2009 Kevin Clague. All rights reserved.
** Copyright (C) 2015 - 2017 Trevor SANDY. All rights reserved.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/****************************************************************************
 *
 * This class encapsulates the external renderers.  For now, this means
 * only ldglite.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include "lpub.h"
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QString>
#include <QStringList>
#include <QPixmap>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include "render.h"
#include "resolution.h"
#include "meta.h"
#include "math.h"
#include "lpub_preferences.h"

#include "paths.h"
//**3D
#include "lc_mainwindow.h"
//**

#ifdef Q_OS_WIN
#include <windows.h>
#endif

Render *renderer;

LDGLite ldglite;
LDView  ldview;
POVRay  povray;


//#define LduDistance 5729.57
#define CA "-ca0.01"
#define USE_ALPHA "+UA"

static double pi = 4*atan(1.0);
// the default camera distance for real size
static float LduDistance = 10.0/tan(0.005*pi/180);

// renderer timeout in milliseconds
static int rendererTimeout(){
    if (Preferences::rendererTimeout == -1)
        return -1;
    else
        return Preferences::rendererTimeout*60*1000;
}

QString fixupDirname(const QString &dirNameIn) {
#ifdef Q_OS_WIN
	long     length = 0;
    TCHAR*   buffer = NULL;
//  30/11/2014 Generating "invalid conversion from const ushort to const wchar" compile error:
//  LPCWSTR dirNameWin = dirNameIn.utf16();
    LPCWSTR dirNameWin = (const wchar_t*)dirNameIn.utf16();

// First obtain the size needed by passing NULL and 0.

    length = GetShortPathName(dirNameWin, NULL, 0);
    if (length == 0){
		qDebug() << "Couldn't get length of short path name, trying long path name\n";
		return dirNameIn;
	}
// Dynamically allocate the correct size 
// (terminating null char was included in length)

    buffer = new TCHAR[length];

// Now simply call again using same long path.

    length = GetShortPathName(dirNameWin, buffer, length);
    if (length == 0){
		qDebug() << "Couldn't get short path name, trying long path name\n";
		return dirNameIn;
	}

	QString dirNameOut = QString::fromWCharArray(buffer);
    
    delete [] buffer;
        return dirNameOut;
#else
        return dirNameIn;
#endif
}

QString const Render::getRenderer()
{
  if (renderer == &ldglite) {
    return "LDGLite";
  } else if (renderer == &ldview){
    return "LDView";
  } else {
    return "POVRay";
  }
}

void Render::setRenderer(QString const &name)
{
  if (name == "LDGLite") {
    renderer = &ldglite;
  } else if (name == "LDView") {
    renderer = &ldview;
  } else {
    renderer = &povray;
  }
}

bool Render::useLDViewSCall(bool override){
  if (override)
    return override;
  else
    return Preferences::useLDViewSingleCall;
}

void clipImage(QString const &pngName){
	//printf("\n");
	QImage toClip(QDir::toNativeSeparators(pngName));
	QRect clipBox = toClip.rect();
	
	//printf("clipping %s from %d x %d at (%d,%d)\n",qPrintable(QDir::toNativeSeparators(pngName)),clipBox.width(),clipBox.height(),clipBox.x(),clipBox.y());
	
	int x,y;
	int initLeft = clipBox.left();
	int initTop = clipBox.top();
	int initRight = clipBox.right();
	int initBottom = clipBox.bottom();
	for(x = initLeft; x < initRight; x++){
		for(y = initTop; y < initBottom; y++){
			QRgb pixel = toClip.pixel(x, y);
			if(!toClip.valid(x,y) || !QColor::fromRgba(pixel).isValid()){
				//printf("something blew up when scanning at (%d,%d) - got %d %d\n",x,y,toClip.valid(x,y),QColor::fromRgba(pixel).isValid());
			}
			if ( pixel != 0){
				//printf("bumped into something at (%d,%d)\n",x,y);
				break;
			}
		}
		if (y != initBottom) {
			clipBox.setLeft(x);
			break;
		}
	}
	
	//printf("clipped to %d x %d at (%d,%d)\n",clipBox.width(),clipBox.height(),clipBox.x(),clipBox.y());
	
	
	initLeft = clipBox.left();
	for(x = initRight; x >= initLeft; x--){
		for(y = initTop; y < initBottom; y++){
			QRgb pixel = toClip.pixel(x, y);
			if(!toClip.valid(x,y) || !QColor::fromRgba(pixel).isValid()){
				//printf("something blew up when scanning at (%d,%d) - got %d %d\n",x,y,toClip.valid(x,y),QColor::fromRgba(pixel).isValid());
			}
			if ( pixel != 0){
				//printf("bumped into something at (%d,%d)\n",x,y);
				break;
			}
		}
		if (y != initBottom) {
			clipBox.setRight(x);
			break;
		}
	}
	
	//printf("clipped to %d x %d at (%d,%d)\n",clipBox.width(),clipBox.height(),clipBox.x(),clipBox.y());
	
	initRight = clipBox.right();
	for(y = initTop; y < initBottom; y++){
		for(x = initLeft; x < initRight; x++){
			QRgb pixel = toClip.pixel(x, y);
			if(!toClip.valid(x,y) || !QColor::fromRgba(pixel).isValid()){
				//printf("something blew up when scanning at (%d,%d) - got %d %d\n",x,y,toClip.valid(x,y),QColor::fromRgba(pixel).isValid());
			}
			if ( pixel != 0){
				//printf("bumped into something at (%d,%d)\n",x,y);
				break;
			}
		}
		if (x != initRight) {
			clipBox.setTop(y);
			break;
		}
	}
	
	//printf("clipped to %d x %d at (%d,%d)\n",clipBox.width(),clipBox.height(),clipBox.x(),clipBox.y());
	
	initTop = clipBox.top();
	for(y = initBottom; y >= initTop; y--){
		for(x = initLeft; x < initRight; x++){
			QRgb pixel = toClip.pixel(x, y);
			if(!toClip.valid(x,y) || !QColor::fromRgba(pixel).isValid()){
				//printf("something blew up when scanning at (%d,%d) - got %d %d\n",x,y,toClip.valid(x,y),QColor::fromRgba(pixel).isValid());
			}
			if ( pixel != 0){
				//printf("bumped into something at (%d,%d)\n",x,y);
				break;
			}
		}
		if (x != initRight) {
			clipBox.setBottom(y);
			break;
		}
	}
	
	//printf("clipped to %d x %d at (%d,%d)\n\n",clipBox.width(),clipBox.height(),clipBox.x(),clipBox.y());
	
	QImage clipped = toClip.copy(clipBox);
	//toClip.save(QDir::toNativeSeparators(pngName+"-orig.png"));
	clipped.save(QDir::toNativeSeparators(pngName));
}

// Shared calculations
float stdCameraDistance(Meta &meta, float scale) {
	float onexone;
	float factor;
	
	// Do the math in pixels
	
	onexone  = 20*meta.LPub.resolution.ldu(); // size of 1x1 in units
	onexone *= meta.LPub.resolution.value();  // size of 1x1 in pixels
	onexone *= scale;
	factor   = gui->pageSize(meta.LPub.page, 0)/onexone; // in pixels;
	
	return factor*LduDistance;
}



/***************************************************************************
 *
 * The math for zoom factor.  1.0 is true size.
 *
 * 1 LDU is 1/64 of an inch
 *
 * LDGLite produces 72 DPI
 *
 * Camera angle is 0.01
 *
 * What distance do we need to put the camera, given a user chosen DPI,
 * to get zoom factor of 1.0?
 *
 **************************************************************************/


/***************************************************************************
 *
 * POVRay renderer
 *
 **************************************************************************/
float POVRay::cameraDistance(
    Meta &meta,
    float scale)
{
  return stdCameraDistance(meta, scale)*0.455;
}

int POVRay::renderCsi(
    const QString     &addLine,
    const QStringList &csiParts,
    const QString     &pngName,
    Meta              &meta)
{
  /* Create the CSI DAT file */
  QString ldrName;
  int rc;
  ldrName = QDir::currentPath() + "/" + Paths::tmpDir + "/csi.ldr";
  QString povName = ldrName + ".pov";
  if ((rc = rotateParts(addLine,meta.rotStep, csiParts, ldrName)) < 0) {
      return rc;
    }

  /* determine camera distance */
  int cd = cameraDistance(meta,meta.LPub.assem.modelScale.value())*1700/1000;
  int width  = gui->pageSize(meta.LPub.page, 0);
  int height = gui->pageSize(meta.LPub.page, 1);

  bool hasSTL       = Preferences::lgeoStlLib;
  bool hasLGEO      = Preferences::lgeoPath != "";
  bool hasPOVRayIni = Preferences::povrayIniPath != "";
  bool hasPOVRayInc = Preferences::povrayIncPath != "";
  bool hasLDViewIni = Preferences::ldviewPOVIni != "";

  QString cg = QString("-cg0.0,0.0,%1") .arg(cd);

  QString w  = QString("-SaveWidth=%1") .arg(width);
  QString h  = QString("-SaveHeight=%1") .arg(height);
  QString f  = QString("-ExportFile=%1") .arg(povName);  // -ExportSuffix not required
  QString l  = QString("-LDrawDir=%1") .arg(fixupDirname(QDir::toNativeSeparators(Preferences::ldrawPath)));

  QStringList arguments;
  arguments << CA;
  arguments << cg;
  arguments << w;
  arguments << h;
  arguments << f;
  arguments << l;
  if(hasLDViewIni){
      QString ini  = QString("-IniFile=%1") .arg(fixupDirname(QDir::toNativeSeparators(Preferences::ldviewPOVIni)));
      arguments << ini;
  }

  QStringList list;
  list = meta.LPub.assem.ldviewParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
      if (list[i] != "" && list[i] != " ") {
          arguments << list[i];
        }
    }
  arguments << ldrName;

  emit gui->messageSig(true, "POVRay render CSI...");

  QProcess    ldview;
  ldview.setEnvironment(QProcess::systemEnvironment());
  ldview.setWorkingDirectory(QDir::currentPath() + "/" + Paths::tmpDir);
  ldview.setStandardErrorFile(QDir::currentPath() + "/stderr-ldviewpov");
  ldview.setStandardOutputFile(QDir::currentPath() + "/stdout-ldivewpov");

  qDebug() << qPrintable(Preferences::ldviewExe + " " + arguments.join(" ")) << "\n";

  ldview.start(Preferences::ldviewExe,arguments);
  if ( ! ldview.waitForFinished(rendererTimeout())) {
      if (ldview.exitCode() != 0) {
          QByteArray status = ldview.readAll();
          QString str;
          str.append(status);
          emit gui->messageSig(false,QMessageBox::tr("LDView POV file generation failed with exit code %1\n%2") .arg(ldview.exitCode()) .arg(str));
          return -1;
        }
    }

  QStringList povArguments;
  if (Preferences::povrayDisplay){
      povArguments << QString("+d");
  } else {
      povArguments << QString("-d");
  }

  QString O = QString("+O\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(pngName)));
  QString I = QString("+I\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(povName)));
  QString W = QString("+W%1").arg(width);
  QString H = QString("+H%1").arg(height);

  povArguments << I;
  povArguments << O;
  povArguments << W;
  povArguments << H;
  povArguments << USE_ALPHA;
  if(hasPOVRayInc){
      QString povinc = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::povrayIncPath)));
      povArguments << povinc;
  }
  if(hasPOVRayIni){
      QString povini = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::povrayIniPath)));
      povArguments << povini;
  }
  if(hasLGEO){
      QString lgeoLg = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::lgeoPath + "/lg")));
      QString lgeoAr = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::lgeoPath + "/ar")));
      povArguments << lgeoLg;
      povArguments << lgeoAr;
      if (hasSTL){
          QString lgeoStl = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::lgeoPath + "/stl")));
          povArguments << lgeoStl;
        }
    }
//#ifndef __APPLE__
//  povArguments << "/EXIT";
//#endif

  list = meta.LPub.assem.povrayParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
      if (list[i] != "" && list[i] != " ") {
          povArguments << list[i];
        }
    }

  emit gui->statusMessage(true, "Execute command: POVRay render CSI.");

  QProcess povray;
  QStringList povEnv = QProcess::systemEnvironment();
  povray.setEnvironment(povEnv);
  povray.setWorkingDirectory(QDir::currentPath()+ "/" + Paths::assemDir); // pov win console app will not write to dir different from cwd or source file dir
  povray.setStandardErrorFile(QDir::currentPath() + "/stderr-povray");
  povray.setStandardOutputFile(QDir::currentPath() + "/stdout-povray");

  qDebug() << qPrintable(Preferences::povrayExe + " " + povArguments.join(" ")) << "\n";

  povray.start(Preferences::povrayExe,povArguments);
  if ( ! povray.waitForFinished(rendererTimeout())) {
      if (povray.exitCode() != 0) {
          QByteArray status = povray.readAll();
          QString str;
          str.append(status);
          emit gui->messageSig(false,QMessageBox::tr("POVRay CSI render failed with code %1\n%2").arg(povray.exitCode()) .arg(str));
          return -1;
        }
    }

  clipImage(pngName);

  return 0;

}

int POVRay::renderPli(
    const QString &ldrName,
    const QString &pngName,
    Meta    	  &meta,
    bool     	  bom)
{
  QString povName = ldrName + ".pov";
  PliMeta &pliMeta = bom ? meta.LPub.bom : meta.LPub.pli;

  /* determine camera distance */
  int cd = cameraDistance(meta,pliMeta.modelScale.value())*1700/1000;
  int width  = gui->pageSize(meta.LPub.page, 0);
  int height = gui->pageSize(meta.LPub.page, 1);

  bool hasSTL       = Preferences::lgeoStlLib;
  bool hasLGEO      = Preferences::lgeoPath != "";
  bool hasPOVRayIni = Preferences::povrayIniPath != "";
  bool hasPOVRayInc = Preferences::povrayIncPath != "";
  bool hasLDViewIni = Preferences::ldviewPOVIni != "";

  //qDebug() << "LDView (Native) Camera Distance: " << cd;

  QString cg = QString("-cg%1,%2,%3") .arg(pliMeta.angle.value(0))
                                      .arg(pliMeta.angle.value(1))
                                      .arg(cd);

  QString w  = QString("-SaveWidth=%1")  .arg(width);
  QString h  = QString("-SaveHeight=%1") .arg(height);
  QString f  = QString("-ExportFile=%1") .arg(povName);  // -ExportSuffix not required
  QString l  = QString("-LDrawDir=%1") .arg(fixupDirname(QDir::toNativeSeparators(Preferences::ldrawPath)));

  QStringList arguments;
  arguments << CA;
  arguments << cg;
  arguments << w;
  arguments << h;
  arguments << f;
  arguments << l;
  if(hasLDViewIni){
      QString ini  = QString("-IniFile=%1") .arg(fixupDirname(QDir::toNativeSeparators(Preferences::ldviewPOVIni)));
      arguments << ini;
  }

  QStringList list;
  list = meta.LPub.pli.ldviewParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
      if (list[i] != "" && list[i] != " ") {
          arguments << list[i];
        }
    }
  arguments << ldrName; //SUSPECT !!! (SHOULD ALSO HAVE povName ?? )

  emit gui->messageSig(true, "POVRay render PLI...");

  QProcess    ldview;
  ldview.setEnvironment(QProcess::systemEnvironment());
  ldview.setWorkingDirectory(QDir::currentPath());
  ldview.setStandardErrorFile(QDir::currentPath() + "/stderr-ldviewpov");
  ldview.setStandardOutputFile(QDir::currentPath() + "/stdout-ldivewpov");

  qDebug() << qPrintable(Preferences::ldviewExe + " " + arguments.join(" ")) << "\n";

  ldview.start(Preferences::ldviewExe,arguments);
  if ( ! ldview.waitForFinished()) {
      if (ldview.exitCode() != 0) {
          QByteArray status = ldview.readAll();
          QString str;
          str.append(status);
          emit gui->messageSig(false,QMessageBox::tr("LDView POV file generation failed with exit code %1\n%2") .arg(ldview.exitCode()) .arg(str));
          return -1;
        }
    }

  QStringList povArguments;
  if (Preferences::povrayDisplay){
      povArguments << QString("+d");
  } else {
      povArguments << QString("-d");
  }

  QString O = QString("+O\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(pngName)));
  QString I = QString("+I\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(povName)));
  QString W = QString("+W%1").arg(width);
  QString H = QString("+H%1").arg(height);

  povArguments << I;
  povArguments << O;
  povArguments << W;
  povArguments << H;
  povArguments << USE_ALPHA;
  if(hasPOVRayInc){
      QString povinc = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::povrayIncPath)));
      povArguments << povinc;
  }
  if(hasPOVRayIni){
      QString povini = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::povrayIniPath)));
      povArguments << povini;
  }
  if(hasLGEO){
      QString lgeoLg = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::lgeoPath + "/lg")));
      QString lgeoAr = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::lgeoPath + "/ar")));
      povArguments << lgeoLg;
      povArguments << lgeoAr;
      if (hasSTL){
          QString lgeoStl = QString("+L\"%1\"").arg(fixupDirname(QDir::toNativeSeparators(Preferences::lgeoPath + "/stl")));
          povArguments << lgeoStl;
        }
    }
//#ifndef __APPLE__
//  povArguments << "/EXIT";
//#endif

  list = meta.LPub.assem.povrayParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
      if (list[i] != "" && list[i] != " ") {
          povArguments << list[i];
        }
    }

  emit gui->statusMessage(true, "Execute command: POVRay render PLI.");

  QProcess povray;
  QStringList povEnv = QProcess::systemEnvironment();
  povray.setEnvironment(povEnv);
  povray.setWorkingDirectory(QDir::currentPath()+ "/" + Paths::partsDir); // pov win console app will not write to dir different from cwd or source file dir
  povray.setStandardErrorFile(QDir::currentPath() + "/stderr-povray");
  povray.setStandardOutputFile(QDir::currentPath() + "/stdout-povray");

  qDebug() << qPrintable(Preferences::povrayExe + " " + povArguments.join(" ")) << "\n";

  povray.start(Preferences::povrayExe,povArguments);
  if ( ! povray.waitForFinished(rendererTimeout())) {
      if (povray.exitCode() != 0) {
          QByteArray status = povray.readAll();
          QString str;
          str.append(status);
          emit gui->messageSig(false,QMessageBox::tr("POVRay PLI render failed with code %1\n%2") .arg(povray.exitCode()) .arg(str));
          return -1;
        }
    }

  clipImage(pngName);

  return 0;
}


/***************************************************************************
 *
 * LDGLite renderer
 *
 **************************************************************************/

float LDGLite::cameraDistance(
  Meta &meta,
  float scale)
{
	return stdCameraDistance(meta,scale);
}

int LDGLite::renderCsi(
  const QString     &addLine,
  const QStringList &csiParts,
  const QString     &pngName,
        Meta        &meta)
{
	/* Create the CSI DAT file */
	QString ldrName;
	int rc;
	ldrName = QDir::currentPath() + "/" + Paths::tmpDir + "/csi.ldr";
	if ((rc = rotateParts(addLine,meta.rotStep, csiParts, ldrName)) < 0) {
		return rc;
	}

  /* determine camera distance */
  
  QStringList arguments;

  int cd = cameraDistance(meta,meta.LPub.assem.modelScale.value());

  int width  = gui->pageSize(meta.LPub.page, 0);
  int height = gui->pageSize(meta.LPub.page, 1);

  QString v  = QString("-v%1,%2")   .arg(width)
                                    .arg(height);
  QString o  = QString("-o0,-%1")   .arg(height/6);
  QString mf = QString("-mF%1")     .arg(pngName);
  
  int lineThickness = resolution()/150+0.5;
  if (lineThickness == 0) {
    lineThickness = 1;
  }
                                    // ldglite always deals in 72 DPI
  QString w  = QString("-W%1")      .arg(lineThickness);

  QString cg = QString("-cg0.0,0.0,%1") .arg(cd);

  arguments << "-l3";               // use l3 parser
  arguments << "-i2";               // image type 2=.png
  arguments << CA;                  // camera FOV angle in degrees
  arguments << cg;                  // camera globe - scale factor
  arguments << "-J";                // perspective projection
  arguments << v;                   // display in X wide by Y high window
  arguments << o;                   // changes the center X across and Y down
  arguments << w;                   // line thickness
  arguments << "-q";                // Anti Aliasing (Quality Lines)

  QStringList list;                 // -fh = Turns on shading mode
  list = meta.LPub.assem.ldgliteParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
    if (list[i] != "" && list[i] != " ") {
      arguments << list[i];
    }
  }

  arguments << mf;                  // .png file name
  arguments << ldrName;             // csi.ldr (input file)

  emit gui->messageSig(true, "Execute command: LDGLite render CSI.");

  QProcess    ldglite;
  QStringList env = QProcess::systemEnvironment();
  env << "LDRAWDIR=" + Preferences::ldrawPath;
  //logDebug() << qPrintable("LDRAWDIR=" + Preferences::ldrawPath);

  if (!Preferences::ldgliteSearchDirs.isEmpty()) {
    env << "LDSEARCHDIRS=" + Preferences::ldgliteSearchDirs;
    //logDebug() << qPrintable("LDSEARCHDIRS: " + Preferences::ldgliteSearchDirs);
  }

  ldglite.setEnvironment(env);
  //logDebug() << qPrintable("ENV: " + env);

  ldglite.setWorkingDirectory(QDir::currentPath() + "/" + Paths::tmpDir);
  ldglite.setStandardErrorFile(QDir::currentPath() + "/stderr-ldglite");
  ldglite.setStandardOutputFile(QDir::currentPath() + "/stdout-ldglite");

  qDebug() << qPrintable("LDGLite CSI Arguments: " + Preferences::ldgliteExe + " " + arguments.join(" ")) << "\n";

  ldglite.start(Preferences::ldgliteExe,arguments);
  if ( ! ldglite.waitForFinished(rendererTimeout())) {
    if (ldglite.exitCode() != 0) {
      QByteArray status = ldglite.readAll();
      QString str;
      str.append(status);
      QMessageBox::warning(NULL,
                           QMessageBox::tr(VER_PRODUCTNAME_STR),
                           QMessageBox::tr("LDGlite failed\n%1") .arg(str));
      return -1;
    }
  }
  //QFile::remove(ldrName);
  return 0;
}

  
int LDGLite::renderPli(
  const QString &ldrName,
  const QString &pngName,
  Meta    &meta,
  bool     bom)
{
  int width  = gui->pageSize(meta.LPub.page, 0);
  int height = gui->pageSize(meta.LPub.page, 1);

  /* determine camera distance */

  PliMeta &pliMeta = bom ? meta.LPub.bom : meta.LPub.pli;

  int cd = cameraDistance(meta,pliMeta.modelScale.value());

  QString cg = QString("-cg%1,%2,%3") .arg(pliMeta.angle.value(0))
                                      .arg(pliMeta.angle.value(1))
                                      .arg(cd);

  QString v  = QString("-v%1,%2")   .arg(width)
                                    .arg(height);
  QString o  = QString("-o0,-%1")   .arg(height/6);
  QString mf = QString("-mF%1")     .arg(pngName);
                                    // ldglite always deals in 72 DPI
  QString w  = QString("-W%1")      .arg(int(resolution()/72.0+0.5));

  QStringList arguments;
  arguments << "-l3";
  arguments << "-i2";
  arguments << CA;
  arguments << cg;
  arguments << "-J";
  arguments << v;
  arguments << o;
  arguments << w;
  arguments << "-q";          //Anti Aliasing (Quality Lines)

  QStringList list;
  list = meta.LPub.pli.ldgliteParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
	  if (list[i] != "" && list[i] != " ") {
      arguments << list[i];
	  }
  }
  arguments << mf;
  arguments << ldrName;
  
  emit gui->messageSig(true, "Execute command: LDGLite render PLI.");

  QProcess    ldglite;
  QStringList env = QProcess::systemEnvironment();
  env << "LDRAWDIR=" + Preferences::ldrawPath;
  //logDebug() << qPrintable("LDRAWDIR=" + Preferences::ldrawPath);

  if (!Preferences::ldgliteSearchDirs.isEmpty()){
    env << "LDSEARCHDIRS=" + Preferences::ldgliteSearchDirs;
    //logDebug() << qPrintable("LDSEARCHDIRS: " + Preferences::ldgliteSearchDirs);
  }

  ldglite.setEnvironment(env);
  ldglite.setWorkingDirectory(QDir::currentPath());
  ldglite.setStandardErrorFile(QDir::currentPath() + "/stderr-ldglite");
  ldglite.setStandardOutputFile(QDir::currentPath() + "/stdout-ldglite");

  qDebug() << qPrintable("LDGLite PLI Arguments: " + Preferences::ldgliteExe + " " + arguments.join(" ")) << "\n";

  ldglite.start(Preferences::ldgliteExe,arguments);
  if (! ldglite.waitForFinished()) {
    if (ldglite.exitCode()) {
      QByteArray status = ldglite.readAll();
      QString str;
      str.append(status);
      QMessageBox::warning(NULL,
                           QMessageBox::tr(VER_PRODUCTNAME_STR),
                           QMessageBox::tr("LDGlite failed\n%1") .arg(str));
      return -1;
    }
  }
  return 0;
}


/***************************************************************************
 *
 * LDView renderer
 *                                  6x6                    5990
 *      LDView               LDView    LDGLite       LDView
 * 0.1    8x5     8x6         32x14    40x19  0.25  216x150    276x191  0.28
 * 0.2   14x10   16x10                              430x298    552x381
 * 0.3   20x14   20x15                              644x466    824x571
 * 0.4   28x18   28x19                              859x594   1100x762
 * 0.5   34x22   36x22                             1074x744   1376x949  0.28
 * 0.6   40x27   40x28                             1288x892
 * 0.7   46x31   48x32                            1502x1040
 * 0.8   54x35   56x37                          
 * 0.9   60x40   60x41
 * 1.0   66x44   68x46       310x135  400x175 0.29 
 * 1.1   72x48
 * 1.2   80x53
 * 1.3   86x57
 * 1.4   92x61
 * 1.5   99x66
 * 1.6  106x70
 * 2.0  132x87  132x90       620x270  796x348 0.28
 * 3.0  197x131 200x134      930x404 1169x522
 * 4.0  262x174 268x178     1238x539 1592x697 0.29
 * 5.0  328x217 332x223     1548x673
 * 
 *
 **************************************************************************/

float LDView::cameraDistance(
  Meta &meta,
  float scale)
{
	return stdCameraDistance(meta, scale)*0.775;
}

int LDView::renderCsi(
  const QString     &addLine,
  const QStringList &csiParts,
  const QString     &pngName,
        Meta        &meta)
{
  /* Create the CSI DAT file */
  QString ldrName;
  int rc;
  ldrName = QDir::currentPath() + "/" + Paths::tmpDir + "/csi.ldr";
  if ((rc = rotateParts(addLine,meta.rotStep, csiParts, ldrName)) < 0) {
      return rc;
    }


  /* determine camera distance */
  int cd = cameraDistance(meta,meta.LPub.assem.modelScale.value())*1700/1000;
  int width  = gui->pageSize(meta.LPub.page, 0);
  int height = gui->pageSize(meta.LPub.page, 1);

  bool hasLDViewIni = Preferences::ldviewIni != "";

  QString cg = QString("-cg0.0,0.0,%1") .arg(cd);

  QString w  = QString("-SaveWidth=%1")  .arg(width);
  QString h  = QString("-SaveHeight=%1") .arg(height);
  QString f  = QString("-SaveSnapShot=%1") .arg(pngName); // -SnapshotSuffix not required
  QString l  = QString("-LDrawDir=%1").arg(Preferences::ldrawPath);

  QStringList arguments;
  arguments << CA;
  arguments << cg;
  arguments << w;
  arguments << h;
  arguments << f;
  arguments << l;
  if(hasLDViewIni){
      QString ini  = QString("-IniFile=%1") .arg(Preferences::ldviewIni);
      arguments << ini;
  }

  QStringList list;
  list = meta.LPub.assem.ldviewParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
    if (list[i] != "" && list[i] != " ") {
      arguments << list[i];
    }
  }
  arguments << ldrName;

  emit gui->messageSig(true, "Execute command: LDView render CSI.");
  
  QProcess    ldview;
  ldview.setEnvironment(QProcess::systemEnvironment());
  ldview.setWorkingDirectory(QDir::currentPath() + "/" + Paths::tmpDir);
  ldview.setStandardErrorFile(QDir::currentPath() + "/stderr-ldview");
  ldview.setStandardOutputFile(QDir::currentPath() + "/stdout-ldview");

  qDebug() << qPrintable("LDView (Native) CSI Arguments: " + Preferences::ldviewExe + " " + arguments.join(" ")) << "\n";

  ldview.start(Preferences::ldviewExe,arguments);
  if ( ! ldview.waitForFinished(rendererTimeout())) {
    if (ldview.exitCode() != 0 || 1) {
      QByteArray status = ldview.readAll();
      QString str;
      str.append(status);
      emit gui->messageSig(false,QMessageBox::tr("LDView CSI render failed with code %1\n%2").arg(ldview.exitCode()) .arg(str));
      return -1;
    }
  }

  return 0;
}

int LDView::renderPli(
  const QString &ldrName,
  const QString &pngName,
  Meta          &meta,
  bool          bom)
{
  QFileInfo fileInfo(ldrName);
  if ( ! fileInfo.exists()) {
    return -1;
  }
  PliMeta &pliMeta = bom ? meta.LPub.bom : meta.LPub.pli;

  /* determine camera distance */
  int cd = cameraDistance(meta,pliMeta.modelScale.value())*1700/1000;
  int width  = gui->pageSize(meta.LPub.page, 0);
  int height = gui->pageSize(meta.LPub.page, 1);

  bool hasLDViewIni = Preferences::ldviewIni != "";

  //qDebug() << "LDView (Native) Camera Distance: " << cd;

  QString cg = QString("-cg%1,%2,%3") .arg(pliMeta.angle.value(0))
                                      .arg(pliMeta.angle.value(1))
                                      .arg(cd);

  QString w  = QString("-SaveWidth=%1")  .arg(width);
  QString h  = QString("-SaveHeight=%1") .arg(height);
  QString f  = QString("-SaveSnapShot=%1") .arg(pngName); // -SnapshotSuffix not required
  QString l  = QString("-LDrawDir=%1").arg(Preferences::ldrawPath);

  QStringList arguments;
  arguments << CA;
  arguments << cg;
  arguments << w;
  arguments << h;
  arguments << f;
  arguments << l;
  if(hasLDViewIni){
      QString ini  = QString("-IniFile=%1") .arg(Preferences::ldviewIni);
      arguments << ini;
  }

  QStringList list;
  list = meta.LPub.pli.ldviewParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
    if (list[i] != "" && list[i] != " ") {
      arguments << list[i];
    }
  }
  arguments << ldrName;

  emit gui->messageSig(true, "Execute command: LDView render PLI.");

  QProcess    ldview;
  ldview.setEnvironment(QProcess::systemEnvironment());
  ldview.setWorkingDirectory(QDir::currentPath());
  ldview.setStandardErrorFile(QDir::currentPath() + "/stderr-ldview");
  ldview.setStandardOutputFile(QDir::currentPath() + "/stdout-ldivew");

  qDebug() << qPrintable("LDView (Native) PLI Arguments: " + Preferences::ldviewExe + " " + arguments.join(" ")) << "\n";

  ldview.start(Preferences::ldviewExe,arguments);
  if ( ! ldview.waitForFinished()) {
    if (ldview.exitCode() != 0) {
      QByteArray status = ldview.readAll();
      QString str;
      str.append(status);
      emit gui->messageSig(false,QMessageBox::tr("LDView PLI render failed with exit code %1\n%2") .arg(ldview.exitCode()) .arg(str));
      return -1;
    }
  }

  return 0;
}

int Render::renderLDViewSCallCsi(
  const QStringList &ldrNames,
        Meta        &meta)
{
  //logInfo() << "LDView SC CSI Renderer Timeout:" << rendererTimeout();

  /* determine camera distance */
  int cd = cameraDistance(meta,meta.LPub.assem.modelScale.value())*1700/1000;
  int width  = gui->pageSize(meta.LPub.page, 0);
  int height = gui->pageSize(meta.LPub.page, 1);

  bool hasLDViewIni = Preferences::ldviewIni != "";

  QString cg = QString("-cg0.0,0.0,%1") .arg(cd);
  QString w  = QString("-SaveWidth=%1") .arg(width);
  QString h  = QString("-SaveHeight=%1") .arg(height);
  QString s  = QString("-SaveSnapShots=1");
  QString t  = QString("-SnapshotSuffix=.png");
  QString l  = QString("-LDrawDir=%1").arg(Preferences::ldrawPath);

  QStringList arguments;
  arguments << CA;
  arguments << cg;
  arguments << w;
  arguments << h;
  arguments << s;
  arguments << t;
  arguments << l;
  if(hasLDViewIni){
      QString ini  = QString("-IniFile=%1") .arg(Preferences::ldviewIni);
      arguments << ini;
  }

  QStringList list;
  list = meta.LPub.assem.ldviewParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
    if (list[i] != "" && list[i] != " ") {
      arguments << list[i];
    }
  }
  arguments = arguments + ldrNames;

  emit gui->messageSig(true, "Execute command: LDView (Single Call) render CSI.");

  QProcess    ldview;
  ldview.setEnvironment(QProcess::systemEnvironment());
  ldview.setWorkingDirectory(QDir::currentPath()+ "/" + Paths::tmpDir);
  ldview.setStandardErrorFile(QDir::currentPath() + "/stderr-ldview");
  ldview.setStandardOutputFile(QDir::currentPath() + "/stdout-ldivew");

  qDebug() << qPrintable("LDView (Single Call) CSI Arguments: " + Preferences::ldviewExe + " " + arguments.join(" ")) << "\n";

  ldview.start(Preferences::ldviewExe,arguments);  
  if ( ! ldview.waitForFinished(rendererTimeout())) {
    if (ldview.exitCode() != 0 || 1) {
      QByteArray status = ldview.readAll();
      QString str;
      str.append(status);
      emit gui->messageSig(false,QMessageBox::tr("LDView (Single Call) CSI render failed with code %1\n%2").arg(ldview.exitCode()) .arg(str));
      return -1;
    }
  }

  // move generated CSI images
  QString ldrName;
  QDir dir(QDir::currentPath() + "/" + Paths::tmpDir);
  foreach(ldrName, ldrNames){
      QFileInfo fInfo(ldrName.replace(".ldr",".png"));
      QString imageFilePath = QDir::currentPath() + "/" +
          Paths::assemDir + "/" + fInfo.fileName();
      if (! dir.rename(fInfo.absoluteFilePath(), imageFilePath)){
          //in case failure because file exist
          QFile pngFile(imageFilePath);
          if (! pngFile.exists()){
              emit gui->messageSig(false,QMessageBox::tr("LDView (Single Call) CSI image file move failed for\n%1").arg(imageFilePath));
              return -1;
            }
        }
    }

  return 0;
}

int Render::renderLDViewSCallPli(
  const QStringList &ldrNames,
  Meta    &meta,
  bool     bom)
{
  //logInfo() << "LDView SC PLI Renderer Timeout:" << rendererTimeout();

  PliMeta &pliMeta = bom ? meta.LPub.bom : meta.LPub.pli;

  /* determine camera distance */
  int cd = cameraDistance(meta,pliMeta.modelScale.value())*1700/1000;
  int width  = gui->pageSize(meta.LPub.page, 0);
  int height = gui->pageSize(meta.LPub.page, 1);

  bool hasLDViewIni = Preferences::ldviewIni != "";

  QString cg = QString("-cg%1,%2,%3") .arg(pliMeta.angle.value(0))
                                      .arg(pliMeta.angle.value(1))
                                      .arg(cd);

  QString w  = QString("-SaveWidth=%1")  .arg(width);
  QString h  = QString("-SaveHeight=%1") .arg(height);
  QString s  = QString("-SaveSnapShots=1");
  QString t  = QString("-SnapshotSuffix=.png");
  QString l  = QString("-LDrawDir=%1").arg(Preferences::ldrawPath);

  QStringList arguments;
  arguments << CA;
  arguments << cg;
  arguments << w;
  arguments << h;
  arguments << s;
  arguments << t;
  arguments << l;
  if(hasLDViewIni){
      QString ini  = QString("-IniFile=%1") .arg(Preferences::ldviewIni);
      arguments << ini;
  }

  QStringList list;
  list = meta.LPub.pli.ldviewParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
    if (list[i] != "" && list[i] != " ") {
      arguments << list[i];
    }
  }
  arguments = arguments + ldrNames;

  emit gui->messageSig(true, "Execute command: LDView (Single Call) render PLI.");

  QProcess    ldview;
  ldview.setEnvironment(QProcess::systemEnvironment());
  ldview.setWorkingDirectory(QDir::currentPath());
  ldview.setStandardErrorFile(QDir::currentPath() + "/stderr-ldview");
  ldview.setStandardOutputFile(QDir::currentPath() + "/stdout-ldivew");

  qDebug() << qPrintable("LDView (Single Call) PLI Arguments: " + Preferences::ldviewExe + " " + arguments.join(" ")) << "\n";

  ldview.start(Preferences::ldviewExe,arguments);  
  if ( ! ldview.waitForFinished(rendererTimeout())) {
      if (ldview.exitCode() != 0) {
          QByteArray status = ldview.readAll();
          QString str;
          str.append(status);
          emit gui->messageSig(false,QMessageBox::tr("LDView (Single Call) PLI render failed with code %1\n%2").arg(ldview.exitCode()) .arg(str));
          return -1;
        }
    }

  // move generated PLIimages
  QString ldrName;
  QDir dir(QDir::currentPath() + "/" + Paths::tmpDir);
  foreach(ldrName, ldrNames){
      QFileInfo fInfo(ldrName.replace(".ldr",".png"));
      QString imageFilePath = QDir::currentPath() + "/" +
          Paths::partsDir + "/" + fInfo.fileName();
      if (! dir.rename(fInfo.absoluteFilePath(), imageFilePath)){
          //in case failure because file exist
          QFile pngFile(imageFilePath);
          if (! pngFile.exists()){
              emit gui->messageSig(false,QMessageBox::tr("LDView (Single Call) PLI image file move failed for\n%1").arg(imageFilePath));
              return -1;
            }
        }
    }

  return 0;
}

//**3D
// create 3D Viewer version of the csi file
// basically integrating subfile content into the csifile because LeoCAD is not smart.

int Render::render3DCsi(
  const QString     &nameKeys,
  const QString     &addLine,
  const QStringList &csiParts,
        Meta        &meta,
        bool        csiExists,
        bool        outOfDate)
{
    QStringList csiSubModels;
    QStringList csiSubModelParts;

    QStringList csi3DParts;
    QString     csi3DName;
    QStringList argv;
    bool        alreadyInserted;
    int         rc;
    bool    doFadeStep  = (gui->page.meta.LPub.fadeStep.fadeStep.value() || Preferences::enableFadeStep);
    QString fadeColor   = LDrawColor::ldColorCode(gui->page.meta.LPub.fadeStep.fadeColor.value());

    csi3DName = QDir::currentPath() + "/" + Paths::viewerDir + "/" + nameKeys;

    if ( ! csiExists || outOfDate) {
        if (csiParts.size() > 0) {
            csi3DParts << "0 FILE " + nameKeys + "\n"
                          "0 !LEOCAD MODEL NAME " + nameKeys;
            for (int index = 0; index < csiParts.size(); index++) {

                alreadyInserted = false;
                QString csiLine = csiParts[index];
                split(csiLine, argv);
                if (argv.size() == 15 && argv[0] == "1") {
                    /* process subfiles in csiParts */
                    QString type = argv[argv.size()-1];

                    bool isFadedItem = (argv[1] == fadeColor && type.contains("-fade."));
                    bool isFadedSubModelOrUnofficialPart = false;
                    if (isFadedItem) {
                        QString fadedType = type;
                        fadedType = fadedType.replace("-fade.",".");
                        isFadedSubModelOrUnofficialPart = (gui->isSubmodel(fadedType) || gui->isUnofficialPart(fadedType));
                      }

//                    logNotice() << " \nROOT - FIRST LEVEL:  "
//                                << " \nCsi3D Part Type:                 " << type
//                                << " \nIsSubmodel:                      " << gui->isSubmodel(type)
//                                << " \nIsUnofficialPart:                " << gui->isUnofficialPart(type)
//                                << " \nIsFadedItem:                     " << isFadedItem
//                                << " \nIsFadedSubModelOrUnofficialPart: " << isFadedSubModelOrUnofficialPart
//                                   ;

                    if (gui->isSubmodel(type) || gui->isUnofficialPart(type) || isFadedSubModelOrUnofficialPart) {
                        /* capture all subfiles (full string) to be processed when finished */
                        foreach (QString csiSubModel, csiSubModels) {
                            if (csiSubModel == type) {
                                alreadyInserted = true;
                                break;
                              } else {
                                alreadyInserted = false;
                              }
                          }

//                        logNotice() << " \nSUB MODEL - FIRST LEVEL:  "
//                                    << " \nCsi3D Part Type:                 " << type
//                                    << " \nIsSubmodel:                      " << gui->isSubmodel(type)
//                                    << " \nIsUnofficialPart:                " << gui->isUnofficialPart(type)
//                                    << " \nIsFadedItem:                     " << isFadedItem
//                                    << " \nIsFadedSubModelOrUnofficialPart: " << isFadedSubModelOrUnofficialPart
//                                    << " \nAlready Inserted:                " << alreadyInserted
//                                       ;

                        if (! alreadyInserted){
                            alreadyInserted = false;
                            csiSubModels << type.toLower();
                          }
                      }
                  }
                csiLine = argv.join(" ");
                csi3DParts << csiLine;
            } //end for

            /* process extracted submodels and unofficial files */
            if (csiSubModels.size() > 0){
                rc = render3DCsiSubModels(csiSubModels, csiSubModelParts, fadeColor, doFadeStep);
                if (rc != 0){
                    QMessageBox::warning(NULL,
                                         QMessageBox::tr(VER_PRODUCTNAME_STR),
                                         QMessageBox::tr("3D-render process extracted submodels failed."));
                    return rc;
                  }
              } else {
                csi3DParts.append("0 NOFILE");
              }

            /* Set the CSI 3D ldr ROTSTEP on top-level content */
            bool viewer = true;
            if ((rc = rotateParts(addLine, meta.rotStep, csi3DParts, csi3DName, viewer)) < 0) {
                QMessageBox::warning(NULL,
                                     QMessageBox::tr(VER_PRODUCTNAME_STR),
                                     QMessageBox::tr("3D-render rotate parts failed for: %1.")
                                     .arg(csi3DName));
                return rc;
            }

            /* add sub model content to csi3D file */
            if (! csiSubModelParts.empty())
            {
                /* append subModel content to csi3D file */
                QFile csi3DFile(csi3DName);
                if ( ! csi3DFile.open(QFile::Append | QFile::Text)) {
                    QMessageBox::warning(NULL,
                                         QMessageBox::tr(VER_PRODUCTNAME_STR),
                                         QMessageBox::tr("Cannot open subModel file %1 for writing:\n%2")
                                         .arg(csi3DName)
                                         .arg(csi3DFile.errorString()));
                    return -1;
                }
                QTextStream out(&csi3DFile);
                for (int i = 0; i < csiSubModelParts.size(); i++) {
                    QString smLine = csiSubModelParts[i];
                    out << smLine << endl;
                }
                out << "0 NOFILE" << endl;
                csi3DFile.close();
            }
        }
    }

    rc = load3DCsiImage(csi3DName);
    if (rc != 0)
      return rc;

    return 0;

}

int Render::render3DCsiSubModels(QStringList &subModels,
                                 QStringList &subModelParts,
                                 QString &fadeColor,
                                 bool doFadeStep)
{
    QStringList csiSubModels        = subModels;
    QStringList csiSubModelParts    = subModelParts;
    QStringList newSubModels;
    QStringList argv;
    bool        alreadyInserted;
    int         rc;

    if (csiSubModels.size() > 0) {

        /* read in all detected sub model file content */
        for (int index = 0; index < csiSubModels.size(); index++) {

            alreadyInserted     = false;
            QString ldrName(QDir::currentPath() + "/" +
                            Paths::tmpDir + "/" +
                            csiSubModels[index]);
            /* initialize the working submodel file - define header. */
            csiSubModelParts.append("0 NOFILE\n0 FILE " + csiSubModels[index] + "\n"
                                    "0 !LEOCAD MODEL NAME " + csiSubModels[index]);
            /* read the actual submodel file */
            QFile ldrfile(ldrName);
            if ( ! ldrfile.open(QFile::ReadOnly | QFile::Text)) {
                QMessageBox::warning(NULL,
                                     QMessageBox::tr(VER_PRODUCTNAME_STR),
                                     QMessageBox::tr("3D CSI render cannot read subModel file %1:\n%2.")
                                     .arg(ldrName)
                                     .arg(ldrfile.errorString()));
                return -1;
            }
            /* populate file contents into working submodel csi parts */
            QTextStream in(&ldrfile);
            while ( ! in.atEnd()) {
                QString csiLine = in.readLine(0);
                split(csiLine, argv);
                if (argv.size() == 15 && argv[0] == "1") {
                    /* check and process any subfiles in csiParts */
                    QString type = argv[argv.size()-1];

                    bool isFadedItem = (argv[1] == fadeColor && type.contains("-fade."));
                    bool isFadedSubModelOrUnofficialPart = false;
                    if (isFadedItem) {
                        QString fadedType = type;
                        fadedType = fadedType.replace("-fade.",".");
                        isFadedSubModelOrUnofficialPart = (gui->isSubmodel(fadedType) || gui->isUnofficialPart(fadedType));
                      }

                    if (gui->isSubmodel(type) || gui->isUnofficialPart(type) || isFadedSubModelOrUnofficialPart) {
                        /* capture all subfiles (full string) to be processed when finished */
                        foreach (QString newSubModel, newSubModels) {
                            if (newSubModel == type) {
                                alreadyInserted = true;
                                break;
                              } else {
                                alreadyInserted = false;
                              }
                          }

//                        logNotice() << " \nSUB MODEL - SECOND LEVEL:  "
//                                    << " \nCsi3D Part Type:                 " << type
//                                    << " \nIsSubmodel:                      " << gui->isSubmodel(type)
//                                    << " \nIsUnofficialPart:                " << gui->isUnofficialPart(type)
//                                    << " \nIsFadedItem:                     " << isFadedItem
//                                    << " \nIsFadedSubModelOrUnofficialPart: " << isFadedSubModelOrUnofficialPart
//                                    << " \nAlready Inserted:                " << alreadyInserted
//                                       ;

                        if (! alreadyInserted){
                            alreadyInserted = false;
                            newSubModels << type;
                          }
                      }
                  }
                csiLine = argv.join(" ");
                csiSubModelParts << csiLine;
            }
        }

        /* recurse and process any identified submodel files */
        if (newSubModels.size() > 0){
            rc = render3DCsiSubModels(newSubModels, csiSubModelParts, fadeColor, doFadeStep);
            if (rc != 0){
                QMessageBox::warning(NULL,
                                     QMessageBox::tr(VER_PRODUCTNAME_STR),
                                     QMessageBox::tr("3D-render recurse submodel file failed."));
                return rc;
              }
          }
        subModelParts = csiSubModelParts;
    }
    return 0;
}

int Render::load3DCsiImage(QString &csi3DName)
{
  //load CSI 3D file into viewer
  QFile csi3DFile(csi3DName);
  if (csi3DFile.exists()){
      if (! gMainWindow->OpenProject(csi3DFile.fileName()))
        return -1;
    } else {
      return -1;
    }

  return 0;
}
