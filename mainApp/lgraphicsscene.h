/****************************************************************************
**
** Copyright (C) 2016 Trevor SANDY. All rights reserved.
**
** This file may be used under the terms of the
** GNU General Public Liceense (GPL) version 3.0
** which accompanies this distribution, and is
** available at http://www.gnu.org/licenses/gpl.html
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef LGRAPHICSSCENE_H
#define LGRAPHICSSCENE_H

#include <QGraphicsScene>

class LGraphicsScene : public QGraphicsScene
{
public:
  LGraphicsScene(QObject *parent = 0);
  QPen guidePen;

  void setGuidesEnabled(bool b){
    guidesEnabled = b;
  }

  bool getGuidesEnabled(){
    return guidesEnabled;
  }

  void setPen(const QPen &pen){
    guidePen = pen;
  }

protected:
  void drawForeground(QPainter* painter, const QRectF& rect);
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
//void mouseReleaseEvent(QGraphicsSceneMouseEvent *event); /* not yet implemented */
private:
  QPointF mMousePos;
  bool guidesEnabled;
};

#endif // LGRAPHICSSCENE_H
