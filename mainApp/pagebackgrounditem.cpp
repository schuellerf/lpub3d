 
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
 * This file represents the page background and is derived from the generic
 * background class described in background.(h,cpp)
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include "pagebackgrounditem.h"
#include <QAction>
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>
#include <QFileDialog>
#include "commonmenus.h"
#include "ranges_element.h"
#include "range.h"
#include "range_element.h"
#include "step.h"
#include "lpub.h"
#include "lpub_preferences.h"
#include "pointer.h"
#include "pagepointeritem.h"

PageBackgroundItem::PageBackgroundItem(
  Page   *_page,
  int     width,
  int     height,
  LGraphicsView *_view)
{
  page = _page;
  view = _view;

  relativeType = page->relativeType;
#if 0
  width = int(page->meta.LPub.page.size.valuePixels(0));
  height= int(page->meta.LPub.page.size.valuePixels(1));
#endif

  pixmap = new QPixmap(width,height);

  QString toolTip("Page background - right-click to modify");

  setBackground(pixmap,
                PageType,
               &page->meta,
                page->meta.LPub.page.background,
                page->meta.LPub.page.border,
                page->meta.LPub.page.margin,
                page->meta.LPub.page.subModelColor,
                page->meta.submodelStack.size(),
                toolTip);

  setPixmap(*pixmap);
  setFlag(QGraphicsItem::ItemIsSelectable,false);
  setFlag(QGraphicsItem::ItemIsMovable,false);

  delete pixmap;
}

void PageBackgroundItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;
  QString name = "Page";
  bool fullContextMenu = page->list.size() && ! page->modelDisplayPage;

  // figure out if first step step number is greater than 1

  QAction *addNextAction = NULL;
  QAction *addPrevAction = NULL;
  QAction *calloutAction = NULL;
  QAction *assembledAction = NULL;
  QAction *ignoreAction = NULL;
  QAction *partAction = NULL;

  QAction *backgroundAction = NULL;
  QAction *sizeAndOrientationAction = NULL;

  QAction *addPointerAction = NULL;

  Step    *lastStep = NULL;
  Step    *firstStep = NULL;

  if (fullContextMenu) {
      AbstractStepsElement *range = page->list[page->list.size()-1];
      if (range->relativeType == RangeType) {
          AbstractRangeElement *rangeElement = range->list[range->list.size()-1];
          if (rangeElement->relativeType == StepType) {
              lastStep = dynamic_cast<Step *> (rangeElement);
              MetaItem mi;
              int numSteps = mi.numSteps(lastStep->topOfStep().modelName);
              if (lastStep->stepNumber.number != numSteps) {
                  addNextAction = menu.addAction("Add Next Step");
                  addNextAction->setIcon(QIcon(":/resources/nextstep.png"));
                  addNextAction->setWhatsThis("Add Next Step:\n Add the first step of the next page to this page\n");
                }
            }
        }

      // figure out if first step step number is greater than 1

      range = page->list[0];
      if (range->relativeType == RangeType) {
          AbstractRangeElement *rangeElement = range->list[0];
          if (rangeElement->relativeType == StepType) {
              firstStep = dynamic_cast<Step *> (rangeElement);
              if (firstStep->stepNumber.number > 1) {
                  addPrevAction = menu.addAction("Add Previous Step");
                  addPrevAction->setIcon(QIcon(":/resources/previousstep.png"));
                  addPrevAction->setWhatsThis("Add Previous Step:\n Add the last step of the previous page to this page\n");
                }
            }
        }

      if (page->meta.submodelStack.size() > 0) {
          calloutAction = menu.addAction("Convert to Callout");
          calloutAction->setIcon(QIcon(":/resources/convertcallout.png"));
          calloutAction->setWhatsThis("Convert to Callout:\n"
                                      "  A callout shows how to build these steps in a picture next\n"
                                      "  to where it is added to the set you are building");

          // FIXME: don't allow this if it already got an assembled.
          if (canConvertToCallout(&page->meta)) {
              assembledAction = menu.addAction("Add Assembled Image to Parent Page");
              assembledAction->setIcon(QIcon(":/resources/addassembledimage.png"));
              assembledAction->setWhatsThis("Add Assembled Image to Parent Page\n"
                                            "  A callout like image is added to the page where this submodel\n"
                                            "  is added to the set you are building");
            }

          ignoreAction = menu.addAction("Ignore this submodel");
          ignoreAction->setIcon(QIcon(":/resources/ignoresubmodel.png"));
          ignoreAction->setWhatsThis("Stops these steps from showing up in your instructions");

          partAction = menu.addAction("Treat as Part");
          partAction->setIcon(QIcon(":/resources/treataspart.png"));
          partAction->setWhatsThis("Treating this submodel as a part means these steps go away, "
                                   "and the submodel is displayed as a part in the parent step's "
                                   "part list image.");
        }
    }

  backgroundAction = commonMenus.backgroundMenu(menu,name);

  sizeAndOrientationAction = menu.addAction("Change Page Size or Orientation");
  sizeAndOrientationAction->setIcon(QIcon(":/resources/pagesizeandorientation.png"));
  sizeAndOrientationAction->setWhatsThis("Change the page size and orientation");

  addPointerAction = menu.addAction("Add Arrow");
  addPointerAction->setWhatsThis("Add arrow from any point on the page to the step where it is used");
  addPointerAction->setIcon(QIcon(":/resources/addarrow.png"));

  QAction *selectedAction     = menu.exec(event->screenPos());

  if (selectedAction == NULL) {
      return;
    }

  bool useTop = relativeType == SingleStepType;

  if (page->meta.LPub.page.background.value().gsize[0] == 0 &&
      page->meta.LPub.page.background.value().gsize[1] == 0) {

      page->meta.LPub.page.background.value().gsize[0] = Preferences::pageHeight;
      page->meta.LPub.page.background.value().gsize[1] = Preferences::pageWidth;

      QSize gSize(page->meta.LPub.page.background.value().gsize[0],
          page->meta.LPub.page.background.value().gsize[1]);
      int h_off = gSize.width() / 10;
      int v_off = gSize.height() / 8;
      page->meta.LPub.page.background.value().gpoints << QPointF(gSize.width() / 2, gSize.height() / 2)
                                                      << QPointF(gSize.width() / 2 - h_off, gSize.height() / 2 - v_off);

    }

  if (fullContextMenu){
      if (selectedAction == calloutAction) {
          convertToCallout(&page->meta, page->bottom.modelName, page->isMirrored, false);
        } else if (selectedAction == assembledAction) {
          convertToCallout(&page->meta, page->bottom.modelName, page->isMirrored, true);
        } else if (selectedAction == ignoreAction) {
          convertToIgnore(&page->meta);
        } else if (selectedAction == partAction) {
          convertToPart(&page->meta);
        } else if (selectedAction == addNextAction) {
          addNextMultiStep(lastStep->topOfSteps(),lastStep->bottomOfSteps());
        } else if (selectedAction == addPrevAction) {
          addPrevMultiStep(firstStep->topOfSteps(),firstStep->bottomOfSteps());
        }
    }

  if (selectedAction == backgroundAction) {
      changeBackground("Page Background",
                       page->top,
                       page->bottom,
                       &page->meta.LPub.page.background, useTop);
    } else if (selectedAction == sizeAndOrientationAction) {
      changeSizeAndOrientation("Size and Orientation",
                               page->top,
                               page->bottom,
                               &page->meta.LPub.page.size,
                               &page->meta.LPub.page.orientation, useTop);
    } else if (selectedAction == addPointerAction) {
      Pointer *pointer = new Pointer(page->top,page->meta.LPub.page.pointer);
      float _loc = 0, _x1 = 0, _y1 = 0, _base = -1, _segments = 1;
      float           _x2 = 0, _y2 = 0;
      float           _x3 = 0, _y3 = 0;
      float           _x4 = 0, _y4 = 0;
      pointer->pointerMeta.setValue(
      PlacementEnc(TopLeft),
      _loc,
      _base,
      _segments,
      _x1,_y1,_x2,_y2,_x3,_y3,_x4,_y4);
      PointerAttributes *pointerBase =
          new PointerAttributes(page->meta.LPub.page.pointerBase);
      PagePointerItem *pagePointer =
          new PagePointerItem(page,pointerBase,pointer,this,view);
      pagePointer->defaultPointer();
    }
}
