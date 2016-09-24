 
/****************************************************************************
**
** Copyright (C) 2007-2009 Kevin Clague. All rights reserved.
** Copyright (C) 2016 Trevor SANDY. All rights reserved.
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
 * This class contains one or more individual range's.
 * By itself, this class represents step groups.  Callouts are derived
 * from ranges.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef rangesH
#define rangesH

#include "pli.h"
#include "meta.h"
#include "placement.h"
#include "resize.h"
#include "rotateiconitem.h"
#include "pageattributepixmapitem.h"
#include "pointer.h"
#include "pagepointeritem.h"
#include "lgraphicsview.h"

/*
 * This is a base class for multi-step and
 * callouts.
 */
class Step;
class AbstractStepsElement;
class Where;
class LGraphicsView;

class Steps : public Placement {
  public:
    Meta           meta;
    Meta           stepGroupMeta;
    QList<AbstractStepsElement *> list;  // of range
    LGraphicsView *view;
    Pli            pli;
    Where          top;                  // needed for non-step pages
    Where          bottom;
    bool           isMirrored;

    Steps();
    Steps(Meta &_meta,LGraphicsView *_view);
   ~Steps();

    QString modelName();
    QString path();
    QString csiName();
    QStringList submodelStack();
    void freeSteps();
    void append(AbstractStepsElement *re);

    virtual AllocEnc allocType();
    virtual AllocMeta &allocMeta();
      
    /* size ranges and place each range */

    void sizeIt();
    void sizeit(AllocEnc allocEnc, int x, int y);
    void sizeitFreeform(int xx, int yy);
    void addGraphicsItems(int ox, int oy, QGraphicsItem *parent);
    virtual void addGraphicsItems(AllocEnc, int ox, int oy, QGraphicsItem *parent);

    Boundary boundary(AbstractStepsElement *);

    const Where &bottomOfStep(AbstractStepsElement *me);
    const Where &topOfSteps();
    const Where &bottomOfSteps();
    void  setTopOfSteps(const Where &tos);
    void  setBottomOfSteps(const Where &bos);
    AbstractStepsElement *nextRange(const AbstractStepsElement *me);
};

#include "render.h"

class Pointer;
class PagePointerItem;
class PageAttributePixmapItem;

class PointerAttributes: public Placement{
public:
  PointerAttributes(PlacementMeta &placementMeta);
  PlacementMeta          placementMeta;
  QGraphicsRectItem     *underpinnings;
};

class Page : public Steps {
  public:
    QList<InsertMeta> inserts;
    QList<InsertPixmapItem *> insertPixmaps;
    QList<PageAttributePixmapItem *> pageAttributePixmaps;
    QList<Pointer *> pointerList;
    QList<PagePointerItem *> graphicsPointerList;
    QList<PointerAttributes *> pointerBaseList;
    bool coverPage;
    bool frontCover;
    bool backCover;
    bool modelDisplayPage;

    Page();
    
    void addInsertPixmap(InsertPixmapItem *pixMap);
    void addPageAttributePixmap(PageAttributePixmapItem *pixMap);
    void freePage();

    /*
     * These functions manage page pointers (Not sure these are needed for page)
     *
     */

    void addGraphicsPointerItem(Pointer *pointer, PointerAttributes *attributes);
    void appendPointer( const Where &here, PointerMeta &pointerMeta);
    void updatePointers( QPoint &delta);
    void drawTips( QPoint &delta);
};



#endif
