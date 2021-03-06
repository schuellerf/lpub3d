#ifndef _LC_GLOBAL_H_
#define _LC_GLOBAL_H_

#include <QtGlobal>
#include <QtOpenGL>
#if QT_VERSION >= QT_VERSION_CHECK(5,4,0)
#include <QtWidgets/QWidget>
#include <QtWidgets>
#include <QOpenGLWidget>
#else
#include <QWidget>
#include <QtGui>
#include <QGLWidget>
#endif

#include <QPrinter>

#undef GL_LINES_ADJACENCY_EXT
#undef GL_LINE_STRIP_ADJACENCY_EXT
#undef GL_TRIANGLES_ADJACENCY_EXT
#undef GL_TRIANGLE_STRIP_ADJACENCY_EXT
#include "lc_glext.h"

// Old defines and declarations.
#define LC_MAXPATH 1024

#define LC_POINTER_TO_INT(p) ((lcint32) (quintptr) (p))

typedef qint8 lcint8;
typedef quint8 lcuint8;
typedef qint16 lcint16;
typedef quint16 lcuint16;
typedef qint32 lcint32;
typedef quint32 lcuint32;
typedef qint64 lcint64;
typedef quint64 lcuint64;
typedef quintptr lcuintptr;

#ifdef Q_OS_WIN
#define snprintf _snprintf
#ifndef _WIN64
#define isnan _isnan
#endif
#if (_MSC_VER >= 1500)
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif
char* strcasestr(const char *s, const char *find);
#else
char* strupr(char* string);
char* strlwr(char* string);
int stricmp(const char* str1, const char* str2);
#endif

// Version number.
#define LC_VERSION_MAJOR 0
#define LC_VERSION_MINOR 82
#define LC_VERSION_PATCH 1
#define LC_VERSION_TEXT "0.82.1"
#define LC_VERSION_BUILD "1867"

// Forward declarations.
class Project;
class lcModel;
class lcObject;
class lcPiece;
class lcCamera;
class lcLight;
class lcGroup;
class PieceInfo;
struct lcPartsListEntry;
struct lcModelPartsEntry;

class lcVector2;
class lcVector3;
class lcVector4;
class lcMatrix33;
class lcMatrix44;

class lcContext;
class lcMesh;
struct lcMeshSection;
struct lcRenderMesh;
class lcTexture;
class lcScene;

class lcFile;
class lcMemFile;
class lcDiskFile;

#endif // _LC_GLOBAL_H_
