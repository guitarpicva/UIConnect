#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream> // only needed for debug
using namespace  std;

static std::pair<double, double> mh2ll(const std::string mh)
{
    double lon = 0.0;
    double lat = 0.0;
    std::vector<uint8_t> ba2;
    for(char c: mh) {
        ba2.push_back((uint8_t)c);
    }
    if(ba2.size()<4) {return std::pair<double, double>(0.0,0.0);}
    if(ba2.size() > 6) { ba2.resize(6);}
    if(ba2.size() > 4 && ba2.size() < 6) {ba2.resize(4);}
//    if(ba2.size() == 4) cout<<"ba2 init:"<<ba2.at(0)<<ba2.at(1)<<ba2.at(2)<<ba2.at(3)<<endl;
//    else if(ba2.size() == 6) cout<<"ba2 init:"<<ba2.at(0)<<ba2.at(1)<<ba2.at(2)<<ba2.at(3)<<ba2.at(4)<<ba2.at(5)<<endl;
    // latitude calcs  sample: FM16dw
    int cc2 = ba2.at(1);//.upper().toHex().toInt(&ok, 16);
    if(cc2 > 90) cc2 -= 32; // make upper case
    //cout<<"cc2 init: "<<cc2<<endl;
    cc2 -= 65; // hex of capitol letter of alphabet - 'A' = deg above South pole * 1/10
    cc2 *= 10; // = degrees above zero at South Pole
    // example: 'M' = 120 deg. North of the South pole
    //cout<<"cc2:"<<cc2<<endl;
    int cc4 = atoi((char *)&ba2.at(3));
    //cout<<"cc4:"<<cc4<<" ba2.size():"<<ba2.size()<<endl;
    if(ba2.size() > 4) {
        double cc6 = 0.0;
        cc6 = ba2.at(5); //.toLower().toHex().toInt(&ok, 16);
        if(cc6 < 97) cc6 += 32;
        //cout<<"cc6:"<<cc6<<endl;
        cc6 -= 97.0;
        //cout<<"cc6-1:"<<cc6;
        cc6 /= 24.0;
        //cout<<"cc6-2:"<<cc6;
        cc6 += (1.0/48.0);
        // cout<<"cc6-3:"<<cc6;
        cc6 -= 90;
        //cout<<"cc6-4:"<<cc6;

        lat = (double)cc2 + (double)cc4 + cc6;
    }
    else {
        //cout<<"short version";
        lat = ((double)cc2 - 90) + (double)cc4;
    }

    // end latitude calcs

    // longitude calcs

    int cc1 = ba2.at(0); //.toUpper().toHex().toInt(&ok, 16);
    if(cc1 > 90) cc1 -= 32; // make upper case
    cc1 -= 65;
    cc1 *= 20;

    //cout<<"cc3:"<<ba2.at(2)<<":"<<atoi((char *)&ba2.at(2))<<endl;
    int cc3 = ba2.at(2) - 48;
    cc3*= 2; //.toInt(&ok, 10) * 2;

    if(ba2.size() > 4) {
        double cc5 = 0;
        cc5 = ba2.at(4); //.toLower().toHex().toInt(&ok, 16);
        if(cc5 < 97) cc5 += 32;// make lower case
        cc5 -= 97.0;
        cc5 /= 12.0;
        cc5 += (1.0/24.0);
        lon = ((double)cc1 + (double) cc3 + cc5) - 180.0;
    }
    else {
        lon = (double)cc1 + (double)cc3 - 180.0;
    }
    // end longitude calcs

    //cout<<"lat:"<<lat<<endl;
    //cout<<cc1<<cc3;
    //cout<<"lon:"<<lon<<endl;
    return std::pair<double, double>(lat, lon);
}

static std::pair<double, double> ll2aprs(const std::pair<double, double> latlon)
{
    double latdeg = (int32_t) latlon.first * 100; // move deg 2 digits left
    double latmin = latlon.first - ((int32_t) latlon.first); // only the fractional part
    //cout<<latdeg<<" "<<latmin<<endl;
    latmin *= 60.0; // move the min 2 dig to the left to add to the deg for APRS
    //cout<<latdeg<<" "<<latmin<<endl;

    double londeg = (int32_t) latlon.second * 100;
    double lonmin = latlon.second - ((int32_t) latlon.second);
    //cout<<londeg<<lonmin<<endl;
    lonmin *= 60;
    //cout<<londeg<<lonmin<<endl;
    return std::pair<double, double>(latdeg+latmin, londeg+lonmin);
}

/**
 * @brief aprs_pos Create a std::string of aprs formatted position using out-
 * put of ll2aprs values.
 * @param latlon - the output of the ll2aprs function
 * @return std::string containing properly formatted std::string of APRS position
 */
static std::string aprs_pos(const std::pair<double, double> latlon) {
    std::string latout, lonout;
    double adj = latlon.first;
    // latitude
    if(latlon.first < 0) {
        adj = abs(adj);
    }
    //cout<<"adj:"<<adj<<endl;
    latout = std::to_string(adj);
    //cout<<"latout:"<<latout<<endl;
    if(adj < 1000.0) {
        latout.insert(0, "0");

    }
    latout = latout.substr(0, 7);
    if(latlon.first < 0) {
        latout.append("S");
    }
    else {
        latout.append("N");
    }
    //cout<<"latout:"<<latout<<endl;

    // now longitude
    if(latlon.second < 0) {
        adj = abs(latlon.second);
    }
    //cout<<"adj:"<<adj<<endl;
    lonout = std::to_string(adj);
    //cout<<"lonout:"<<lonout<<endl;
    if(adj < 10000.0) {
        lonout.insert(0, "0");
    }
    lonout = lonout.substr(0, 8);
    //cout<<"lonout:"<<lonout<<endl;
    if(latlon.second < 0) {
        lonout.append("W");
    }
    else {
        lonout.append("E");
    }

    return latout.append("/").append(lonout);
}
