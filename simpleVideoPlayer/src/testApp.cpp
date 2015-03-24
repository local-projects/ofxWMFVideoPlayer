#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
	playing = false;
	ofSetFrameRate(55);
	video.loadMovie("http://www.cloudsdocumentary.com/uploads/Aaron_code,_story,_communication.mov");
	video.play();
	
}

//--------------------------------------------------------------
void testApp::update(){
	video.update();

}

//--------------------------------------------------------------
void testApp::draw(){
	//if(playing){
		video.draw(0,0);
	//}
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
	switch(key)
	{
		case ' ':
			{
				video.play();
				playing = true;
			}
		case '1':	
			{
				video.loadMovie("test.mp4");
				video.play();
				break;
			}
		case '2':	
			{
				video.loadMovie("test2.mp4");
				video.play();
				break;
			}
		
	}

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}
