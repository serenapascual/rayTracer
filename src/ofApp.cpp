/*
 Serena Pascual
 2018 Oct 22
 */

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetBackgroundColor(ofColor::black);
	gui.setup();
	gui.add(radiusParam.set("Radius", 0.2, 0.1, 5.0));
	radiusParam.addListener(this, &ofApp::onRadiusChanged);
	gui.add(intensityParam.set("Intensity", 0.85, 0, 1));
	intensityParam.addListener(this, &ofApp::onIntensityChanged);
	gui.add(pSlider.setup("Power", 30, 10, 10000));
	
	bHide = false;
	mainCam.setDistance(15);
	theCam = &mainCam;
	
	sideCam.setPosition(50, 0, 0);
	sideCam.lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	
	previewCam.setPosition(0, 0, 15);
	previewCam.lookAt(glm::vec3(0, 0, -1));
	
	scene.push_back(new Plane(glm::vec3(0, -2, 0), glm::vec3(0, 1, 0), ofColor::darkOrchid));
	shapeCount += 1; // TODO: Make createShape() versatile enough to accommodate planes
//	createShape(glm::vec3(0.0, 0.0, 2.0), 2.0, ofColor(0, 0, 0));
//	createShape(glm::vec3(2.0, 0.0, 0.0), 1.75, ofColor(1, 1, 1));
	createShape(glm::vec3(0.0, 0.0, 2.0), 2.0, ofColor::orangeRed);
	createShape(glm::vec3(2.0, 0.0, 0.0), 1.75, ofColor::cornflowerBlue);
	createShape(glm::vec3(-3.0, 0.0, -1.5), 1.5, ofColor::paleGreen);
	createLight(glm::vec3(5, -1, -3), 0.25, 0.4, ofColor::white);
	createLight(glm::vec3(-2, 5, 6), 0.1, 0.8, ofColor::white);
	
	ofImage texture1, texture2;
	texture1.load("texture1.jpeg");
	texture2.load("texture2.jpeg");
	textures.push_back(texture1);
	textures.push_back(texture2);

}

//--------------------------------------------------------------
void ofApp::update(){
	settingRadius = true;
	if (selectedObj != NULL) {
		radiusParam = selectedObj->getRadius();
	}
	settingRadius = false;
	
	settingIntensity = true;
	if (selectedObj != NULL) {
		intensityParam = selectedObj->getIntensity();
	}
	settingIntensity = false;
}

//--------------------------------------------------------------
void ofApp::draw(){
	if (!bHide) gui.draw();
	theCam->begin();
	
	ofNoFill();
	for (vector<SceneObject *>::iterator i = scene.begin(); i != scene.end(); ++i) {
		(*i)->draw();
	}
	for (vector<Light *>::iterator i = lights.begin(); i != lights.end(); ++i) {
		(*i)->draw();
	}
	
	for (int j = 0; j < renderCam.view.height(); j++) {
		for (int i = 0; i < renderCam.view.width(); i++) {
			float u = (i + 0.5) / renderCam.view.width();
			float v = (j + 0.5) / renderCam.view.height();
			Ray ray = renderCam.getRay(u, v);
			ofSetColor(ofColor::blue);
			if (bMouseDown) ray.draw(100);
		}
	}
	
	drawGrid(); // Slow if image size is greater than 6px by 4px
	
	mainCam.draw();
	renderCam.draw();
	renderCam.view.draw();
	
	theCam->end();
	
	//    image.draw(0, 0);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key) {
		case OF_KEY_SHIFT:
			bShift = true;
			break;
		case OF_KEY_CONTROL:
			bCtrl = true;
			break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
	switch (key) {
		case 'C':
		case 'c':
			if (mainCam.getMouseInputEnabled()) mainCam.disableMouseInput();
			else mainCam.enableMouseInput();
			break;
		case 'F':
		case 'b':
			break;
		case 'd':
			if (selectedObj) {
				deleteObject(selectedObj);
			}
			break;
		case 'f':
			ofToggleFullscreen();
			break;
		case 'h':
			bHide = !bHide;
			break;
		case 'i':
			break;
		case 'l':
			createLight();
			break;
		case 'r':
			rayTrace();
			break;
		case 's':
			createShape();
			break;
		case OF_KEY_F1:
			theCam = &mainCam;
			break;
		case OF_KEY_F2:
			theCam = &sideCam;
			break;
		case OF_KEY_F3:
			theCam = &previewCam;
			break;
		case OF_KEY_SHIFT:
			bShift = false;
			break;
		case OF_KEY_CONTROL:
			bCtrl = false;
			break;
	}
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){
	
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	if (selectedObj && bDrag) {
		glm::vec3 point;
		mouseToWorld(x, y, point);
		selectedObj->position += (point - lastPoint);
		lastPoint = point;
	}
}

//--------------------------------------------------------------
bool ofApp::mouseToWorld(int x, int y, glm::vec3 &point) {
	glm::vec3 p = theCam->screenToWorld(glm::vec3(x, y, 0));
	glm::vec3 d = p - theCam->getPosition();
	glm::vec3 dn = glm::normalize(d);
	
	float dist;
	
	if (glm::intersectRayPlane(p, d, selectedObj->position, glm::normalize(theCam->getZAxis()), dist)) {
		point = p + d * dist;
		return true;
	}
	return false;
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	bMouseDown = true;
	
	//
	// test if something selected
	//
	vector<SceneObject *> selected;
	
	glm::vec3 p = theCam->screenToWorld(glm::vec3(x, y, 0));
	glm::vec3 d = p - theCam->getPosition();
	glm::vec3 dn = glm::normalize(d);
	
	// Check for selection of scene objects
	//
	for (int i = 0; i < scene.size(); i++) {
		
		glm::vec3 point, norm;
		
		//  We hit a non-light object
		//
		if (scene[i]->intersect(Ray(p, dn), point, norm)) {
			selected.push_back(scene[i]);
			selectedObj = scene[i];
		}
	}
	
	for (int i = 0; i < lights.size(); i++) {
		
		glm::vec3 point, norm;
		
		//  We hit a light
		//
		if (lights[i]->intersect(Ray(p, dn), point, norm)) {
			selected.push_back(lights[i]);
			selectedObj = lights[i];
		}
	}
	
	// if we selected more than one, pick nearest
	//
	if (selected.size() > 1) {
		float nearestDist = std::numeric_limits<float>::infinity();
		SceneObject *nearestObj = NULL;
		for (int n = 0; n < selected.size(); n++) {
			float dist = glm::length(selected[n]->position - theCam->getPosition());
			if (dist < nearestDist) {
				nearestDist = dist;
				selectedObj = selected[n];
			}
		}
	}
	if (selected.size() == 0) {
		selectedObj = NULL;
	}
	else { // An object is selected
		bDrag = true;
		// If selected object has radius attribute, reflect in slider
		if (selectedObj->getRadius() != -1) {
			radiusParam = selectedObj->getRadius();
		}
		// If selected object has intensity attribute, reflect in slider
		if (selectedObj->getIntensity() != -1) {
			intensityParam = selectedObj->getIntensity();
		}
		
		mouseToWorld(x, y, lastPoint);
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	bMouseDown = false;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){
	
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
	
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
	
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
	
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
	
}

//--------------------------------------------------------------
void ofApp::rayTrace() {
	image.allocate(imageWidth, imageHeight, OF_IMAGE_COLOR);
	for (int j = 0; j < imageHeight; j++) {
		for (int i = 0; i < imageWidth; i++) {
			float u = (float(i) + 0.5) / float(imageWidth);
			float v = (float(j) + 0.5) / float(imageHeight);
			
			Ray ray = renderCam.getRay(u, v);
			
			glm::vec3 intersection, normal;
			bool hit = false;
			int nearestObj = -1;
			
			// initialize to a very big number
			float nearestDist = std::numeric_limits<float>::infinity();
			
			for (int n = 0; n < scene.size(); n++) {
				if (scene[n]->intersect(ray, intersection, normal)) {
					float dist = glm::length(intersection - renderCam.position);
					
					if (dist < nearestDist) {
						nearestDist = dist;
						nearestObj = n;
					}
					hit = true;
				}
			}
			
			// Set the color of the pixel to the nearest object's pixel
			// if we didn't hit anything, set it the bg color.
			// "Unflip" image by adjust in the "j" direction.
			if (!hit) {
				image.setColor(i, imageHeight - j - 1, ofColor::black);
			}
			else {
				// If the nearest object is the first one, i.e. the plane, use Phong shading
				if (nearestObj == 0) {
					ofColor phongColor = phong(intersection, normal, scene[nearestObj]->diffuseColor, ofColor::white, pSlider);
					image.setColor(i, imageHeight - j - 1, phongColor);
				}
				// Otherwise use texture mapping
				else {
					ofColor phongTexture;
					phongTexture = phong(intersection, normal, textureLookup(textures[int((scene[nearestObj]->diffuseColor).r)], u, v), ofColor::white, pSlider); // only works for two spheres for now
					image.setColor(i, imageHeight - j - 1, phongTexture);
				}
			}
		}
	}
	image.save("out.png");
}

/** ANTIALIASING RAY TRACE */
////--------------------------------------------------------------
//void ofApp::rayTrace() {
//	image.allocate(imageWidth, imageHeight, OF_IMAGE_COLOR);
//	int samples = 4;
//	for (int j = 0; j < imageHeight; j++) {
//		for (int i = 0; i < imageWidth; i++) {
//			ofColor antialiased = ofColor(0.0f, 0.0f, 0.0f);
//			for (int p = 0; p < samples; p++) {
//				for (int q = 0; q < samples; q++) {
//					float u = (float(i) + (p + randomEpsilon()) / float(samples)) / float(imageWidth);
//					float v = (float(j) + (q + randomEpsilon()) / float(samples)) / float(imageHeight);
//					Ray ray = renderCam.getRay(u, v);
//
//					glm::vec3 intersection, normal;
//					bool hit = false;
//					int nearestObj = -1;
//
//					// initialize to a very big number
//					float nearestDist = std::numeric_limits<float>::infinity();
//
//					for (int n = 0; n < scene.size(); n++) {
//						if (scene[n]->intersect(ray, intersection, normal)) {
//							float dist = glm::length(intersection - renderCam.position);
//
//							if (dist < nearestDist) {
//								nearestDist = dist;
//								nearestObj = n;
//							}
//							hit = true;
//						}
//					}
//
//					// Set the color of the pixel to the nearest object's pixel
//					// if we didn't hit anything, set it the bg color.
//					// "Unflip" image by adjust in the "j" direction.
//					if (!hit) {
////						image.setColor(i, imageHeight - j - 1, ofColor::black);
//						antialiased = ofColor::black;
//					}
//					else {
//						//	ofColor lambertColor = lambert(intersection, normal, scene[nearestObj]->diffuseColor);
//						ofColor phongColor = phong(intersection, normal, scene[nearestObj]->diffuseColor, ofColor::white, pSlider);
//						antialiased += phongColor / (samples * samples);
//					}
//				}
//			}
//			image.setColor(i, imageHeight - j - 1, antialiased);
//		}
//	}
//	image.save("out.png");
//}

//--------------------------------------------------------------
void ofApp::drawGrid() {
	float f = 0;
	int viewWidth = renderCam.view.width();
	while (f < viewWidth) {
		ofSetColor(ofColor::white);
		glm::vec3 p1 = glm::vec3(renderCam.view.min.x + f, renderCam.view.min.y, 5);
		glm::vec3 p2 = glm::vec3(renderCam.view.min.x + f, renderCam.view.max.y, 5);
		ofDrawLine(p1, p2);
		f += viewWidth / imageWidth;
	}
	
	float g = 0;
	int viewHeight = renderCam.view.height();
	while (g < viewHeight) {
		ofSetColor(ofColor::white);
		glm::vec3 p1 = glm::vec3(renderCam.view.min.x, renderCam.view.min.y + g, 5);
		glm::vec3 p2 = glm::vec3(renderCam.view.max.x, renderCam.view.min.y + g, 5);
		ofDrawLine(p1, p2);
		g += viewWidth / imageWidth;
	}
}

//--------------------------------------------------------------
void ofApp::drawAxis(glm::vec3 position) {
	ofPushMatrix();
	ofTranslate(position);
	
	ofSetLineWidth(1.0);
	
	// X Axis
	ofSetColor(ofColor(255, 0, 0));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(1, 0, 0));
	
	
	// Y Axis
	ofSetColor(ofColor(0, 255, 0));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 1, 0));
	
	// Z Axis
	ofSetColor(ofColor(0, 0, 255));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 0, 1));
	
	ofPopMatrix();
}

// Intersect Ray with Plane  (wrapper on glm::intersect*
//
bool Plane::intersect(const Ray &ray, glm::vec3 &point, glm::vec3 &normalAtIntersect) {
	float dist;
	bool hit = glm::intersectRayPlane(ray.p, ray.d, position, this->normal, dist);
	if (hit) {
		// Set output variables
		Ray r = ray;
		point = r.evalPoint(dist);
		normalAtIntersect = this->normal;
		// If ray hits plane, determine if intersection is within bounds of the plane
		glm::vec2 xBounds = glm::vec2(position.x - width * 0.5, position.x + width * 0.5);
		glm::vec2 zBounds = glm::vec2(position.z - height * 0.5, position.z + height * 0.5);
		if (point.x < xBounds[1] && point.x > xBounds[0] && point.z < zBounds[1] && point.z > zBounds[0]) {
			return true;
		}
	}
	return false;
}

// Convert (u, v) to (x, y, z)
// We assume u,v is in [0, 1]
//
glm::vec3 ViewPlane::toWorld(float u, float v) {
	float w = width();
	float h = height();
	return (glm::vec3((u * w) + min.x, (v * h) + min.y, position.z));
}

// Get a ray from the current camera position to the (u, v) position on
// the ViewPlane
//
Ray RenderCam::getRay(float u, float v) {
	glm::vec3 pointOnPlane = view.toWorld(u, v);
	return(Ray(position, glm::normalize(pointOnPlane - position)));
}

//--------------------------------------------------------------
ofColor ofApp::lambert(SceneObject* light, const glm::vec3 &p, const glm::vec3 &norm, const ofColor diffuse) {
	glm::vec3 l, n;
	n = glm::normalize(norm);
	float dot, intensity;
	l = glm::normalize(light->position - p);
	dot = glm::dot(n, l);
	intensity = light->intensity;
	return diffuse * intensity * glm::max(0.0f, dot);
}

//--------------------------------------------------------------
ofColor ofApp::phong(const glm::vec3 &p, const glm::vec3 &norm, const ofColor diffuse, const ofColor specular, float power) {
	ofColor shadedColor = ofColor(0, 0, 0);
	glm::vec3 l, v, h, n;
	n = glm::normalize(norm);
	float dot, intensity;
	for (int i = 0; i < lights.size(); i++) {
		l = glm::normalize(lights[i]->position - p);
		v = glm::normalize(renderCam.position - p);
		h = glm::normalize(v + l);
		dot = glm::dot(n, h);
		shadedColor += lambert(lights[i], p, norm, diffuse); // lambert shading
		shadedColor += specular * lights[i]->intensity * glm::pow(glm::max(0.0f, dot), power); // blinn-phong
	}
	return shadedColor;
}

// Listens to radius change on slider and updates selected object radius accordingly
//
//--------------------------------------------------------------
void ofApp::onRadiusChanged(float &r) {
	if (settingRadius) return; // If the radius is being changed, return
	else {
		if (selectedObj != NULL) selectedObj->setRadius(r);
	}
}

// Listens to intensity change on slider and updates selected object intensity accordingly
//
//--------------------------------------------------------------
void ofApp::onIntensityChanged(float &i) {
	if (settingIntensity) return; // If the intensity is being changed, return
	else {
		if (selectedObj != NULL) selectedObj->setIntensity(i);
	}
}

//--------------------------------------------------------------
void ofApp::createShape() {
	scene.push_back(new Sphere(glm::vec3(0, 0, 0), 1.0, ofColor::darkGoldenRod, scene.size()));
	shapeCount += 1;
}

//--------------------------------------------------------------
void ofApp::createShape(glm::vec3 p, float r, ofColor d) {
	scene.push_back(new Sphere(p, r, d, scene.size()));
	shapeCount += 1;
}

//--------------------------------------------------------------
void ofApp::createLight() {
	lights.push_back(new Light(glm::vec3(0, 5, 0), 0.2, 0.85, ofColor::white, lights.size()));
	lightCount += 1;
}

//--------------------------------------------------------------
void ofApp::createLight(glm::vec3 p, float r, float i, ofColor d) {
	lights.push_back(new Light(p, r, i, d, lights.size()));
	lightCount += 1;
}

//--------------------------------------------------------------
void ofApp::deleteObject(SceneObject *o) {
	bool isLight = false;
	// Cheap way of determining whether selected object is a light or a shape
	if (o->getIntensity() != -1) {
		isLight = true;
	}
	
	// If selected object is a light, delete it from the light vector
	if (isLight) {
		// Update ordinality of all lights after the light to be deleted
		for (int i = o->ordinality + 1; i < lights.size(); i++) {
			lights[i]->ordinality -= 1;
		}
		
		// Remove the selected light from the list of lights
		lights.erase(lights.begin() + o->ordinality);
	}
	// Otherwise, selected object is a shape; delete it from the scene vector
	else {
		// Update ordinality of all shapes after the shape to be deleted
		for (int i = o->ordinality + 1; i < scene.size(); i++) {
			scene[i]->ordinality -= 1;
		}
		// Remove the selected shape from the list of shapes
		scene.erase(scene.begin() + o->ordinality);
	}
	
	// Clear selection
	selectedObj = NULL;
}

// Returns a random uniform number in the range [0, 1)
//
//--------------------------------------------------------------
float ofApp::randomEpsilon() {
	float epsilon = 0.001;
	int max = 999;
	int min = 0;
	int random = rand() % (max - min + 1); // Generates random number between 0 and 99
	epsilon *= random; // Brings random number into range of [0, 1)
	return epsilon;
}


// Converts texture coordinates (u, v) to the color at texel coordinates (i, j)
//
//--------------------------------------------------------------
ofColor ofApp::textureLookup(ofImage img, float u, float v) {
	int i = int(u * img.getWidth() - 0.5);
	int j = int(v * img.getHeight() - 0.5);
	return img.getColor(i % int(img.getWidth()), j % int(img.getHeight()));
}
