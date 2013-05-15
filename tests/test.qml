import Qt 4.7
import Gst 1.0

Item {

    SystemPalette { id:activePalette }

    width: 320
    height: 240

    // black background
    Rectangle {
    	      anchors.fill: parent
	      color: "black"
    }

    // the main video item, uses the surface set on the root context
    // in main.cpp
    
    VideoItem {
    	      id:video
    	      surface: videosurface
	      size: Qt.size(parent.width, parent.height)

    }

    
    MouseArea {
    	      anchors.fill: parent

	      hoverEnabled: true

	      onPositionChanged: {
		  controls.opacity = 0.5
		  hideControls.restart()
	      }
    }
    
    Rectangle {

    	      id: controls
	      
    	      anchors {
		      left: parent.left
		      right: parent.right
		      bottom: parent.bottom
	      }
	      height: 25
	      
	      color: "white"
	      opacity: 0.5

	      Behavior on opacity {
	      	       NumberAnimation {
		           duration: 200
		       }
	      }

	      Row {
	          id:buttons 
	      	  anchors {
		  	  
			  left: parent.left
			  top: parent.top

			  topMargin: 3
			  bottomMargin: 3
			  leftMargin: 3
			  rightMargin: 3
		  }

		  spacing: 6

		  Button {
		     text: "Play"
		  }
	      }

	      Rectangle {
	         anchors {
		    right: parent.right
		    left: buttons.right
		    top: parent.top
		    bottom: parent.bottom
		    topMargin: 10
		    bottomMargin: 10
		    rightMargin: 5
		    leftMargin: 5
		 }

		 color: "red"
	      }

    }

    Timer {
       id: hideControls
       interval: 4000
       running: true

       onTriggered: {
          controls.opacity = 0.0
       }
    }

}
