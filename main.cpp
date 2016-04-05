//
//  main.cpp
//  ParkingSpace
//
//  Created by synycboom on 9/11/2558 BE.
//  Copyright © 2558 HOME. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <array>
#include <opencv2/opencv.hpp>
#include <opencv2/ml/ml.hpp>
#include "opencv2/features2d/features2d.hpp"
#include <opencv2/features2d.hpp>
#include "opencv2/xfeatures2d/nonfree.hpp"
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "SVMTest.cpp"
#include "HistogramTool.cpp"
using namespace cv;
using namespace std;
using namespace ml;

int h_min_slider;
int s_min_slider;
int v_min_slider;

int h_max_slider;
int s_max_slider;
int v_max_slider;

Mat img1;
Point yellowMarker, blueMarker, greenMarker, pinkMarker;
Mat yellowMat, greenMat, pinkMat, blueMat;
bool lostCount[] = {false, false, false, false};

void on_change () {
    Mat HSVImage;
    Mat processedImage;
    
    cvtColor(img1, HSVImage, CV_BGR2HSV); //convert image to HSV and save into HSVImage
    inRange(HSVImage, Scalar(h_min_slider,s_min_slider,v_min_slider),
            Scalar(h_max_slider,s_max_slider,v_max_slider), processedImage);
    
    imshow("Processed Image", processedImage);
    
    cout << "Min :: " << h_min_slider << " :: " << s_min_slider << " :: " << v_min_slider << endl;
    cout << "Max :: " << h_max_slider << " :: " << s_max_slider << " :: " << v_max_slider << endl;
    waitKey(30);
}

static void on_Hchange (int value,void *userData) {
    h_min_slider = value;
    on_change();
}

static void on_Schange (int value, void *userData) {
    s_min_slider = value;
    on_change();
}

static void on_Vchange (int value, void *userData) {
    v_min_slider = value;
    on_change();
}

static void on_max_Hchange (int value,void *userData) {
    h_max_slider = value;
    on_change();
}

static void on_max_Schange (int value, void *userData) {
    s_max_slider = value;
    on_change();
}

static void on_max_Vchange (int value, void *userData) {
    v_max_slider = value;
    on_change();
}

void showTrackbarHSV(){
    namedWindow("Trackbars",1);
    createTrackbar("max_H", "Trackbars", 0, 255,on_max_Hchange,&h_max_slider);
    createTrackbar("max_S", "Trackbars", 0, 255,on_max_Schange,&s_max_slider);
    createTrackbar("max_V", "Trackbars", 0, 255,on_max_Vchange,&v_max_slider);
    createTrackbar("min_H", "Trackbars", 0, 255,on_Hchange,&h_min_slider);
    createTrackbar("min_S", "Trackbars", 0, 255,on_Schange,&s_min_slider);
    createTrackbar("min_V", "Trackbars", 0, 255,on_Vchange,&v_min_slider);
    waitKey(0);
}

void getMarkerPos(Mat img1, Mat img2, vector<Point> &marker){
    
    vector<KeyPoint> keypoints1;
    vector<KeyPoint> keypoints2;
    Mat des1, des2;
    
    int minHessian = 600;
    
    Ptr<xfeatures2d::SURF> detector = xfeatures2d::SURF::create( minHessian );
    
    detector->detect( img1, keypoints1 );
    detector->detect( img2, keypoints2 );
    
    detector->detectAndCompute(img1, Mat(), keypoints1, des1);
    detector->detectAndCompute(img2, Mat(), keypoints2, des2);
    
    FlannBasedMatcher matcher;
    std::vector< DMatch > matches;
    matcher.match( des1, des2, matches );
    
    double max_dist = 0; double min_dist = 100;
    
    for( size_t i = 0; i < des1.rows; i++ ){
        double dist = matches[i].distance;
        if( dist < min_dist ) min_dist = dist;
        if( dist > max_dist ) max_dist = dist;
    }
    
    std::vector< DMatch > good_matches;
    
    for( size_t i = 0; i < des1.rows; i++ ){
        if( matches[i].distance <= max(2*min_dist, 0.02) ){
            good_matches.push_back( matches[i]);
        }
    }
    

//    Mat img_matches;
//    drawMatches( img1, keypoints1, img2, keypoints2,
//                good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
//                vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

    
    for( int i = 0; i < good_matches.size(); i++ ){
//        printf( "-- Good Match [%d] Keypoint 1: %d  -- Keypoint 2: %d  \n", i, good_matches[i].queryIdx, good_matches[i].trainIdx );
        Point tmp((int) keypoints1[good_matches[i].queryIdx].pt.x,
                  (int) keypoints1[good_matches[i].queryIdx].pt.y);
        marker.push_back(tmp);
    }
    
//    imshow( "Good Matches", img_matches );
//    waitKey(0);
    
}

//enum DistanceBetween { NONE ,FAR, NEAR, NORMAL };

struct MARKERINFO{
    Point u;
    Point v;
    double distance;
};

struct MARKERSET{
    Point u;
    Point v;
};

bool calculateOtherMarkerPosition(Mat &output){
    
    int exist = 0;
    vector<Point> _marker;
    
    Point I;
    Point J;
    Point K;
    
    MARKERINFO A;
    MARKERINFO B;
    MARKERINFO C;
    
    MARKERSET farSet;
    MARKERSET normalSet;
    MARKERSET nearSet;
    
    if(yellowMarker.x == 0 && yellowMarker.y ==0)
        lostCount[0] = true;
    else
        _marker.push_back(yellowMarker);
                           
    if(blueMarker.x == 0 && blueMarker.y ==0)
        lostCount[1] = true;
    else
        _marker.push_back(blueMarker);
    
    if(greenMarker.x == 0 && greenMarker.y ==0)
        lostCount[2] = true;
    else
        _marker.push_back(greenMarker);
    
    if(pinkMarker.x == 0 && pinkMarker.y ==0)
        lostCount[3] = true;
    else
        _marker.push_back(pinkMarker);
    
    for(size_t i = 0; i < 4; i++){
        if(!lostCount[i])
            exist++;
    }
    
    if(exist <= 2) return false;
    if(exist == 4) return true;
    if(exist == 3){
        
        A.u = _marker[0];
        A.v = _marker[1];
        A.distance = norm(A.u - A.v);
        
        B.u = _marker[1];
        B.v = _marker[2];
        B.distance = norm(B.u - B.v);
    
        C.u = _marker[0];
        C.v = _marker[2];
        C.distance = norm(C.u - C.v);
        
        vector<double> distanceVect;
        
        distanceVect.push_back(A.distance);
        distanceVect.push_back(B.distance);
        distanceVect.push_back(C.distance);
        
        sort(distanceVect.begin(), distanceVect.end());
        reverse(distanceVect.begin(), distanceVect.end());
        
        for(size_t i = 0; i < distanceVect.size(); i++){
            double _distance = distanceVect[i];
            
            if(i == 0){
                if(_distance == A.distance){
                    farSet.u = A.u;
                    farSet.v = A.v;
                }
                if(_distance == B.distance){
                    farSet.u = B.u;
                    farSet.v = B.v;
                }
                if(_distance == C.distance){
                    farSet.u = C.u;
                    farSet.v = C.v;
                }
            }
            if(i == 1){
                if(_distance == A.distance){
                    normalSet.u = A.u;
                    normalSet.v = A.v;
                }
                if(_distance == B.distance){
                    normalSet.u = B.u;
                    normalSet.v = B.v;
                }
                if(_distance == C.distance){
                    normalSet.u = C.u;
                    normalSet.v = C.v;
                }
            }
            if(i == 2){
                if(_distance == A.distance){
                    nearSet.u = A.u;
                    nearSet.v = A.v;
                }
                if(_distance == B.distance){
                    nearSet.u = B.u;
                    nearSet.v = B.v;
                }
                if(_distance == C.distance){
                    nearSet.u = C.u;
                    nearSet.v = C.v;
                }
            }
        }
        
        if(nearSet.u == normalSet.u || nearSet.u == normalSet.v){
            I = (nearSet.u == normalSet.u) ? normalSet.v : normalSet.u;
            J = nearSet.u;
            K = nearSet.v;
        }
        if(nearSet.v == normalSet.u || nearSet.v == normalSet.v){
            I = (nearSet.v == normalSet.u) ? normalSet.v : normalSet.u;
            J = nearSet.v;
            K = nearSet.u;
        }
        Point tmp = I + (K - J);
        
        circle(output, tmp, 10, Scalar(0,0,255));
        cout << tmp << endl;
        
    }
    
    return true;
    
    
    
}

int main(int argc, const char * argv[]) {
    
//    img1 = imread(DataManager::getInstance().FULL_PATH_PHOTO + "Marking_day3/6.JPG");
    img1 = imread(DataManager::getInstance().FULL_PATH_PHOTO + "Marking4/1.png");
    Mat marker1 = imread(DataManager::getInstance().FULL_PATH_PHOTO + "Marking_day3/marker1.png");
    Mat marker2 = imread(DataManager::getInstance().FULL_PATH_PHOTO + "Marking_day3/marker2.png");
    Mat marker3 = imread(DataManager::getInstance().FULL_PATH_PHOTO + "Marking_day3/marker3.png");
    
    vector<Point> markerPos;
    vector<Point> posToDel;

    Mat HSVImage;
    cvtColor(img1, HSVImage, CV_BGR2HSV); //convert image to HSV and save into HSVImage
    
    
    getMarkerPos(img1, marker1,markerPos);
    getMarkerPos(img1, marker2,markerPos);
    getMarkerPos(img1, marker3,markerPos);
    
    inRange(HSVImage, Scalar(46,52,197), Scalar(81,255,255), greenMat);
    inRange(HSVImage, Scalar(94,127,228), Scalar(116,255,255), blueMat);
    inRange(HSVImage, Scalar(114,45,179), Scalar(153,255,255), pinkMat);
    inRange(HSVImage, Scalar(26,34,204), Scalar(40,255,255), yellowMat);

    Mat output = greenMat + blueMat + pinkMat + yellowMat;

    
    cvtColor(greenMat, greenMat, CV_GRAY2BGR);
    cvtColor(blueMat, blueMat, CV_GRAY2BGR);
    cvtColor(pinkMat, pinkMat, CV_GRAY2BGR);
    cvtColor(yellowMat, yellowMat, CV_GRAY2BGR);
    cvtColor(output, output, CV_GRAY2BGR);
    
//    for(size_t i = 0; i < markerPos.size(); i++){
//        circle(greenMat, markerPos[i], 10, Scalar(0,0,255));
//    }
    
    double yellowSum = 0, blueSum = 0, greenSum = 0, pinkSum = 0;

    int offset = 10;
    int area = 1000;
    
    for (size_t i = 0; i < markerPos.size(); i++){
        //check green point
        Mat green_(greenMat, Rect(markerPos[i].x - offset, markerPos[i].y - offset, offset * 2 , offset * 2));
        Mat orange_(blueMat, Rect(markerPos[i].x - offset, markerPos[i].y - offset, offset * 2 , offset * 2));
        Mat yellow_(yellowMat, Rect(markerPos[i].x - offset, markerPos[i].y - offset, offset * 2 , offset * 2));
        Mat pink_(pinkMat, Rect(markerPos[i].x - offset, markerPos[i].y - offset, offset * 2 , offset * 2));

        if(cv::sum( yellow_ )[0] > area && cv::sum( yellow_ )[0] > yellowSum){
            yellowSum = cv::sum( yellow_ )[0];
            yellowMarker = markerPos[i];
            cout << "yellow: " << yellowMarker << endl;
        }
        
        if(cv::sum( orange_ )[0] > area && cv::sum( orange_ )[0] > blueSum){
            blueSum = cv::sum( orange_ )[0];
            blueMarker = markerPos[i];
            cout << "orange: " << blueMarker << endl;
        }
        
        if(cv::sum( green_ )[0] > area && cv::sum( green_ )[0] > greenSum){
            greenSum = cv::sum( green_ )[0];
            greenMarker = markerPos[i];
            cout << "green: " << greenMarker << endl;
        }
        
        if(cv::sum( pink_ )[0] > area && cv::sum( pink_ )[0] > pinkSum){
            pinkSum = cv::sum( pink_ )[0];
            pinkMarker = markerPos[i];
            cout << "pink: " << pinkMarker << endl;
        }
    }
    
    
    calculateOtherMarkerPosition(output);
    
    circle(output, yellowMarker, 10, Scalar(0,0,255));
    circle(output, blueMarker, 10, Scalar(0,0,255));
    circle(output, greenMarker, 10, Scalar(0,0,255));
    circle(output, pinkMarker, 10, Scalar(0,0,255));
    
    imshow("asd", output);
    waitKey(0);
    
    
}