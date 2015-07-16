/* Buzz Machine Tester
 * Copyright (C) 2015 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.h"
#include <windows.h>
#include "BuzzMachineLoader.h"

#ifdef _MSC_VER
#define DI __declspec(dllimport)
#else
#define DI
#endif

// prototypes

extern "C" DI void bm_set_master_info(long bpm, long tpb, long srat);

extern "C" DI void bm_close(BuzzMachineHandle *bmh);
extern "C" DI BuzzMachineHandle *bm_open(char *bm_file_name);

extern "C" DI int bm_get_machine_info(BuzzMachineHandle *bmh,BuzzMachineProperty key,void *value);
extern "C" DI int bm_get_global_parameter_info(BuzzMachineHandle *bmh,int index,BuzzMachineParameter key,void *value);
extern "C" DI int bm_get_track_parameter_info(BuzzMachineHandle *bmh,int index,BuzzMachineParameter key,void *value);
extern "C" DI int bm_get_attribute_info(BuzzMachineHandle *bmh,int index,BuzzMachineAttribute key,void *value);
extern "C" DI char const *bm_describe_global_value(BuzzMachineHandle *bmh,int const param,int const value);
extern "C" DI char const *bm_describe_track_value(BuzzMachineHandle *bmh,int const param,int const value);

extern "C" DI void bm_free(BuzzMachine *bm);
extern "C" DI BuzzMachine *bm_new(BuzzMachineHandle *bmh);
extern "C" DI void bm_init(BuzzMachine *bm, unsigned long blob_size, unsigned char *blob_data);

extern "C" DI int bm_get_track_parameter_value(BuzzMachine *bm,int track,int index);
extern "C" DI int bm_get_global_parameter_value(BuzzMachine *bm,int index);
extern "C" DI int bm_get_attribute_value(BuzzMachine *bm,int index);

static inline double
_get_timestamp (void)
{
  SYSTEMTIME st;
  GetSystemTime(&st);

  return ((double) st.wSecond + ((double) st.wMilliseconds * 1.0e-3));
}

int _tmain(int argc, _TCHAR* argv[])
{
  void *bmh,*bm;
  char *str;
  int type,val,i,num,tracks,numtrig;
  int maval,mival,noval;
  double ts1, ts2;
  char *libpath="C:\\Programme\\Jeskola Buzz\\Gear\\Effects\\cheapo amp.dll";

  bm_set_master_info (120, 4, 44100);

  ts1=_get_timestamp();

  if((bmh=bm_open(libpath))) {
    if((bm=bm_new(bmh))) {
      char *machine_types[]={"MT_MASTER","MT_GENERATOR","MT_EFFECT" };
      char *parameter_types[]={"PT_NOTE","PT_SWITCH","PT_BYTE","PT_WORD" };

      ts2=_get_timestamp();
      printf("  machine created in %lf sec\n",ts2-ts1);
      ts1=ts2;
      bm_init(bm,0,NULL);
      ts2=_get_timestamp();
      printf("  machine initialized in %lf sec\n",ts2-ts1);

      if(bm_get_machine_info(bmh,BM_PROP_SHORT_NAME,(void *)&str))           printf("    Short Name: \"%s\"\n",str);
      if(bm_get_machine_info(bmh,BM_PROP_NAME,(void *)&str))                 printf("    Name: \"%s\"\n",str);
      if(bm_get_machine_info(bmh,BM_PROP_AUTHOR,(void *)&str))               printf("    Author: \"%s\"\n",str);
      if(bm_get_machine_info(bmh,BM_PROP_COMMANDS,(void *)&str)) {
        if(str) {
          char *t=strdup(str), *p=t;
          while(*p) {
            if(*p=='\n') *p=',';
            p++;
          }
          printf("    Commands: \"%s\"\n",t);
          free(t);
        }
      }
      if(bm_get_machine_info(bmh,BM_PROP_TYPE,(void *)&val))                 printf("    Type: %i -> \"%s\"\n",val,((val<3)?machine_types[val]:"unknown"));
      if(bm_get_machine_info(bmh,BM_PROP_VERSION,(void *)&val))              printf("    Version: %3.1f\n",(float)val/10.0);
      if(bm_get_machine_info(bmh,BM_PROP_FLAGS,(void *)&val)) {              printf("    Flags: 0x%x\n",val);
        if(val&(1<<0)) puts("      MIF_MONO_TO_STEREO");
        if(val&(1<<1)) puts("      MIF_PLAYS_WAVES");
        if(val&(1<<2)) puts("      MIF_USES_LIB_INTERFACE");
        if(val&(1<<3)) puts("      MIF_USES_INSTRUMENTS");
        if(val&(1<<4)) puts("      MIF_DOES_INPUT_MIXING");
        if(val&(1<<5)) puts("      MIF_NO_OUTPUT");
        if(val&(1<<6)) puts("      MIF_CONTROL_MACHINE");
        if(val&(1<<7)) puts("      MIF_INTERNAL_AUX");
        //if(val&) puts("      ");
      }
      if(bm_get_machine_info(bmh,BM_PROP_MIN_TRACKS,(void *)&val))           printf("    MinTracks: %i\n",val);
      tracks=val;
      if(bm_get_machine_info(bmh,BM_PROP_MAX_TRACKS,(void *)&val))           printf("    MaxTracks: %i\n",val);
      if(bm_get_machine_info(bmh,BM_PROP_NUM_INPUT_CHANNELS,(void *)&val))   printf("    InputChannels: %d\n",val);
      if(bm_get_machine_info(bmh,BM_PROP_NUM_OUTPUT_CHANNELS,(void *)&val))  printf("    OutputChannels: %d\n",val);fflush(stdout);
      if(bm_get_machine_info(bmh,BM_PROP_NUM_GLOBAL_PARAMS,(void *)&val)) {  printf("    NumGlobalParams: %i\n",val);fflush(stdout);
        num=val;numtrig=0;
        for(i=0;i<num;i++)
          if(bm_get_global_parameter_info(bmh,i,BM_PARA_FLAGS,(void *)&val))
            if(val&&(1<<1)==0) numtrig++;
        printf("    NumGlobalTriggerParams: %i\n",numtrig);fflush(stdout);
        for(i=0;i<num;i++) {
          printf("      GlobalParam=%02i\n",i);
          if(bm_get_global_parameter_info(bmh,i,BM_PARA_TYPE,(void *)&type))        printf("        Type: %i -> \"%s\"\n",type,((type<4)?parameter_types[type]:"unknown"));
          if(bm_get_global_parameter_info(bmh,i,BM_PARA_NAME,(void *)&str))         printf("        Name: \"%s\"\n",str);
          if(bm_get_global_parameter_info(bmh,i,BM_PARA_DESCRIPTION,(void *)&str))  printf("        Description: \"%s\"\n",str);
          if(bm_get_global_parameter_info(bmh,i,BM_PARA_FLAGS,(void *)&val)) {      printf("        Flags: 0x%x\n",val);
            if(val&(1<<0)) puts("          MPF_WAVE");
            if(val&(1<<1)) puts("          MPF_STATE");
            if(val&(1<<2)) puts("          MPF_TICK_ON_EDIT");
            //if(val&) puts("      ");
          }
          if(bm_get_global_parameter_info(bmh,i,BM_PARA_MIN_VALUE,(void *)&mival) &&
             bm_get_global_parameter_info(bmh,i,BM_PARA_MAX_VALUE,(void *)&maval) &&
             bm_get_global_parameter_info(bmh,i,BM_PARA_NO_VALUE,(void *)&noval) &&
             bm_get_global_parameter_info(bmh,i,BM_PARA_DEF_VALUE,(void *)&val)) {
            printf("        Value: %d .. %d .. %d [%d]\n",mival,val,maval,noval);
            val =        bm_get_global_parameter_value(bm,i);
            str =(char *)bm_describe_global_value(bmh,i,val);
            printf("        RealValue: %d %s (NULL)\n",val,str);
          }
        }
      }
      else {
        puts("    NumGlobalTriggerParams: 0");
      }
      fflush(stdout);
      if(bm_get_machine_info(bmh,BM_PROP_NUM_TRACK_PARAMS,(void *)&val)) {   printf("    NumTrackParams: %i\n",val);fflush(stdout);
        num=val;numtrig=0;
        for(i=0;i<num;i++)
          if(bm_get_track_parameter_info(bmh,i,BM_PARA_FLAGS,(void *)&val))
            if(val&&(1<<1)==0) numtrig++;
        printf("    NumTrackTriggerParams: %i\n",numtrig);fflush(stdout);
        if(num && tracks) {
          for(i=0;i<num;i++) {
            printf("      TrackParam=%02i\n",i);
            if(bm_get_track_parameter_info(bmh,i,BM_PARA_TYPE,(void *)&type))        printf("        Type: %i -> \"%s\"\n",type,((type<4)?parameter_types[type]:"unknown"));
            if(bm_get_track_parameter_info(bmh,i,BM_PARA_NAME,(void *)&str))         printf("        Name: \"%s\"\n",str);
            if(bm_get_track_parameter_info(bmh,i,BM_PARA_DESCRIPTION,(void *)&str))  printf("        Description: \"%s\"\n",str);
            if(bm_get_track_parameter_info(bmh,i,BM_PARA_FLAGS,(void *)&val)) {      printf("        Flags: 0x%x\n",val);
              if(val&(1<<0)) puts("          MPF_WAVE");
              if(val&(1<<1)) puts("          MPF_STATE");
              if(val&(1<<2)) puts("          MPF_TICK_ON_EDIT");
              //if(val&) puts("      ");
            }
			if(bm_get_track_parameter_info(bmh,i,BM_PARA_MIN_VALUE,(void *)&mival) &&
               bm_get_track_parameter_info(bmh,i,BM_PARA_MAX_VALUE,(void *)&maval) &&
               bm_get_track_parameter_info(bmh,i,BM_PARA_NO_VALUE,(void *)&noval) &&
               bm_get_track_parameter_info(bmh,i,BM_PARA_DEF_VALUE,(void *)&val)) {
              printf("        Value: %d .. %d .. %d [%d]\n",mival,val,maval,noval);
              val =        bm_get_track_parameter_value(bm,0,i);
              str =(char *)bm_describe_track_value(bmh,i,val);
              printf("        RealValue: %d %s (NULL)\n",val,str);
            }
          }
        }
        else {
          if(num) {
            printf("      WARNING but tracks=0..0\n");fflush(stdout);
          }
        }
      }
      else {
        puts("    NumTrackTriggerParams: 0");
      }
      fflush(stdout);
      if(bm_get_machine_info(bmh,BM_PROP_NUM_ATTRIBUTES,(void *)&val)) {     printf("    NumAttributes: %i\n",val);fflush(stdout);
        num=val;
        for(i=0;i<num;i++) {
          printf("      Attribute=%02i\n",i);
          if(bm_get_attribute_info(bmh,i,BM_ATTR_NAME,(void *)&str))         printf("        Name: \"%s\"\n",str);
          if(bm_get_attribute_info(bmh,i,BM_ATTR_MIN_VALUE,(void *)&mival) &&
             bm_get_attribute_info(bmh,i,BM_ATTR_MAX_VALUE,(void *)&maval) &&
             bm_get_attribute_info(bmh,i,BM_ATTR_DEF_VALUE,(void *)&val)) {
            printf("        Value: %d .. %d .. %d\n",mival,val,maval);
            val=bm_get_attribute_value(bm,i);printf("        RealValue: %d\n",val);
          }
        }
      }
      bm_free(bm);

      puts("  done");
    } else {
      printf("%s: can't instantiate plugin \"%s\"\n",__FUNCTION__,libpath);
    }
    bm_close(bmh);
  } else {
    printf("%s: can't open plugin \"%s\"\n",__FUNCTION__,libpath);
  }

  system("pause");
  return 0;
}

