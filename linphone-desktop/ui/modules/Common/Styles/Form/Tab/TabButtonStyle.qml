pragma Singleton
import QtQuick 2.7

import Common 1.0

// =============================================================================

QtObject {
  property QtObject backgroundColor: QtObject {
    property color hovered: Colors.s
    property color normal: Colors.i
    property color pressed: Colors.t
    property color selected: Colors.k
  }

  property QtObject text: QtObject {
    property int fontSize: 9
    property int height: 40
    property int leftPadding: 10
    property int rightPadding: 10

    property QtObject color: QtObject {
      property color hovered: Colors.k
      property color normal: Colors.k
      property color pressed: Colors.k
      property color selected: Colors.i
    }
  }
}