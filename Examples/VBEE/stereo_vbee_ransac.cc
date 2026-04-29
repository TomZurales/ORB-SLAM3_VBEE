/**
* This file is part of ORB-SLAM3
*
* Copyright (C) 2017-2021 Carlos Campos, Richard Elvira, Juan J. Gómez Rodríguez, José M.M. Montiel and Juan D. Tardós, University of Zaragoza.
* Copyright (C) 2014-2016 Raúl Mur-Artal, José M.M. Montiel and Juan D. Tardós, University of Zaragoza.
*
* ORB-SLAM3 is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
* License as published by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM3 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
* the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with ORB-SLAM3.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include "vbee_manager.h"
#include<iostream>
#include<algorithm>
#include<fstream>
#include<iomanip>
#include<chrono>
#include<unordered_set>

#include<opencv2/core/core.hpp>

#include<System.h>

using namespace std;

void LoadImages(const string &strPathLeft, const string &strPathRight, const string &strPathTimes,
                vector<string> &vstrImageLeft, vector<string> &vstrImageRight, vector<double> &vTimeStamps);

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        cerr << endl << "Usage: ./stereo_vbee_baseline dataset" << endl;
        return 1;
    }

    const char* datasetDir = getenv("DATASET_DIR");
    if(!datasetDir)
    {
        cerr << endl << "DATASET_DIR environment variable is not set." << endl;
        return 1;
    }

    const string dataset(argv[1]);
    const string basePath = string(datasetDir) + "/" + dataset;

    const string vocabPath      = string(datasetDir) + "/ORBvoc.txt";
    const string settingsPath   = basePath + "/settings.yaml";
    const string pathCam0       = basePath + "/mav0/cam0/data";
    const string pathCam1       = basePath + "/mav0/cam1/data";
    const string pathTimeStamps = basePath + "/timestamps.txt";

    // Load images
    vector<string> vstrImageLeft, vstrImageRight;
    vector<double> vTimestampsCam;

    cout << "Loading images...";
    LoadImages(pathCam0, pathCam1, pathTimeStamps, vstrImageLeft, vstrImageRight, vTimestampsCam);
    cout << "LOADED!" << endl;

    // Load split timestamps
    unordered_set<string> splitStamps;
    {
        ifstream fSplits(basePath + "/splits.txt");
        string s;
        while(getline(fSplits, s))
            if(!s.empty()) splitStamps.insert(s);
    }

    const int nImages = vstrImageLeft.size();

    // Vector for tracking time statistics
    vector<float> vTimesTrack(nImages);

    cout << endl << "-------" << endl;
    cout.precision(17);

    VBEE::Manager* pVBEEManager = new VBEE::Manager(true, true);

    // Create SLAM system. It initializes all system threads and gets ready to process frames.
    ORB_SLAM3::System SLAM(vocabPath, settingsPath, ORB_SLAM3::System::STEREO, pVBEEManager, false);

    cv::Mat imLeft, imRight;
    for(int ni=0; ni<nImages; ni++)
    {
        imLeft  = cv::imread(vstrImageLeft[ni],  cv::IMREAD_UNCHANGED);
        imRight = cv::imread(vstrImageRight[ni], cv::IMREAD_UNCHANGED);

        if(imLeft.empty())
        {
            cerr << endl << "Failed to load image at: " << vstrImageLeft[ni] << endl;
            return 1;
        }

        if(imRight.empty())
        {
            cerr << endl << "Failed to load image at: " << vstrImageRight[ni] << endl;
            return 1;
        }

        double tframe = vTimestampsCam[ni];

#ifdef COMPILEDWITHC11
        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
#else
        std::chrono::monotonic_clock::time_point t1 = std::chrono::monotonic_clock::now();
#endif

        SLAM.TrackStereo(imLeft, imRight, tframe, vector<ORB_SLAM3::IMU::Point>(), vstrImageLeft[ni]);

#ifdef COMPILEDWITHC11
        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
#else
        std::chrono::monotonic_clock::time_point t2 = std::chrono::monotonic_clock::now();
#endif

#ifdef REGISTER_TIMES
        double t_track = std::chrono::duration_cast<std::chrono::duration<double,std::milli>>(t2 - t1).count();
        SLAM.InsertTrackTime(t_track);
#endif

        double ttrack = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
        vTimesTrack[ni] = ttrack;

        // Wait to load the next frame
        double T = 0;
        if(ni < nImages-1)
            T = vTimestampsCam[ni+1] - tframe;
        else if(ni > 0)
            T = tframe - vTimestampsCam[ni-1];

        if(ttrack < T)
            usleep((T-ttrack)*1e6);

        // Check if this frame is a split point
        const string &imgPath = vstrImageLeft[ni];
        const string tsStr = imgPath.substr(imgPath.rfind('/')+1, imgPath.rfind('.')-imgPath.rfind('/')-1);
        if(splitStamps.count(tsStr))
        {
            cout << "Split point reached at timestamp " << tsStr << endl;
            SLAM.EndEpisode();
        }
    }

    SLAM.EndEpisode();

    pVBEEManager->saveVBEEStats("VBEEStats.csv");

    SLAM.Shutdown();

    SLAM.SaveTrajectoryEuRoC("CameraTrajectory.txt");
    SLAM.SaveKeyFrameTrajectoryEuRoC("KeyFrameTrajectory.txt");

    return 0;
}

void LoadImages(const string &strPathLeft, const string &strPathRight, const string &strPathTimes,
                vector<string> &vstrImageLeft, vector<string> &vstrImageRight, vector<double> &vTimeStamps)
{
    ifstream fTimes;
    fTimes.open(strPathTimes.c_str());
    vTimeStamps.reserve(5000);
    vstrImageLeft.reserve(5000);
    vstrImageRight.reserve(5000);
    while(!fTimes.eof())
    {
        string s;
        getline(fTimes,s);
        if(!s.empty())
        {
            stringstream ss;
            ss << s;
            vstrImageLeft.push_back(strPathLeft + "/" + ss.str() + ".png");
            vstrImageRight.push_back(strPathRight + "/" + ss.str() + ".png");
            double t;
            ss >> t;
            vTimeStamps.push_back(t/1e9);

        }
    }
}
