#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <string.h>
#include <queue>

int main(){
  vector<vector<string> > X;
  vector<vector<string> > T;
  readSentences(X, T, "dse.txt"); // dse.txt or ese.txt

  unordered_map<string, set<uint> > sentenceIds;
  set<string> allDocs; //Store the name of all docs
  ifstream in("sentenceid.txt");
  string line;
  uint numericId = 0;

  //map the numeric Id to the file locations
  while(getline(in, line)) {
    vector<string> s = split(line, ' ');
    assert(s.size() == 3);
    string strId = s[2];

    if (sentenceIds.find(strId) != sentenceIds.end()) {
      sentenceIds[strId].insert(numericId);
    } else {
      sentenceIds[strId] = set<uint>();
      sentenceIds[strId].insert(numericId);
    }
    numericId++;
  }

  vector<vector<string> > trainX, validX, testX;
  vector<vector<string> > trainL, validL, testL;
  vector<bool> isUsed(X.size(), false);

  ifstream in4("datasplit/doclist.mpqaOriginalSubset");
  while(getline(in4, line))
    allDocs.insert(line);

  ifstream in2("datasplit/filelist_train"+to_string(fold));
  //10 fold
  while(getline(in2, line)) {
    for (const auto &id : sentenceIds[line]) {
      trainX.push_back(X[id]);
      trainL.push_back(T[id]);
    }
    allDocs.erase(line);
  }
  ifstream in3("datasplit/filelist_test"+to_string(fold));
  while(getline(in3, line)) {
    for (const auto &id : sentenceIds[line]) {
      testX.push_back(X[id]);
      testL.push_back(T[id]);
    }
    allDocs.erase(line);
  }

}

