#ifndef __MP4UTILS_H_
#define __MP4UTILS_H_

int getAACTrack(MP4FileHandle);
int getAudioTrack(MP4FileHandle);
int getVideoTrack(MP4FileHandle);
void getMP4info(char*);

#endif
