#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
	playing = false;
	ofSetFrameRate(55);
//	video.loadMovie("http://www.cloudsdocumentary.com/uploads/Aaron_code,_story,_communication.mov");
//	video.loadMovie("https://d2jijm80bx6z70.cloudfront.net/clouds/videos/6621/31840/Golan_formalism.mov?response-content-disposition=attachment;filename=%22Golan_formalism%201080p.mp4%22&Expires=1427499938&Signature=Yxc3qAlireMg7ok7nSGlij-FCsftAonvGJ~U942BUzZWI6t-2~misOWjwygBuP7Cw60h7XCtw4iVD97OeJviicjYXC28syvN4oG4ODE516d0AndmpZS31HGsoLp9F0N6LIlnt01d3tIAzgRnVpWY~Il0p5lRtzZTrbnwp105xodO2e7cXI2sTXnTu1XxltwlS-RIQUr4gM~nArJriMVf1J7FkLzvvF6-77dgO4YZtfHzRMyzg4Ky8M8eVdBdspGK-ZWTQDOg~3vQRuWyKHE1p-Am5fSMwCJYAO8hRFg4F6KpwW~~XZMkcFurLAbLvtN-UVPwU7qEdTLhw-9LOLX1-w__&Key-Pair-Id=APKAIKJG7ZVXLCEDZ7RQ");
	video.loadMovie("Aaron_code,_story,_communication.mov");
	video.play();
	
}

//--------------------------------------------------------------
void testApp::update(){
	
	//cout << " position is " << video.getPosition() << " " << video.getDuration() << endl; 
	//video.setVolume(ofMap(mouseX, 0, ofGetWidth(), 0, 1.0));
	video.update();

}

//--------------------------------------------------------------
void testApp::draw(){
	//ofBackground( 255 * (sin(ofGetElapsedTimef()*10)*.5+.5) );

	//if(playing){
	video.draw(0,0);
	//}
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
	switch(key)
	{

		case 's':
			video.setPosition(20.0);
			break;
		case ' ':
			{
				video.play();
				playing = true;
				break;
			}
		case '1':	
			{
			//	video.loadMovie("http://www.cloudsdocumentary.com/uploads/Aaron_code,_story,_communication.mov");
			//	video.loadMovie("Aaron_code,_story,_communication.mov");
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
