/* Buzz Machine Loader
 * Copyright (C) 2009 Buzztrax team <buzztrax-devel@buzztrax.org>
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

int bml(test_process(char *libpath,const char *infilename,const char *outfilename)) {
  int okay=0;
  // buzz machine handle
  void *bmh,*bm;

  /*
  {
    fpu_control_t cw=0x27F;
    //_FPU_GETCW(cw);
    //cw&=~_FPU_EXTENDED;
    //cw|=_FPU_DOUBLE;
    _FPU_SETCW(cw);
  }
  */

  printf("%s(\"%s\")\n",__FUNCTION__,libpath);

  if((bmh=bml(open(libpath)))) {
    if((bm=bml(new(bmh)))) {
      FILE *infile,*outfile;
      int s_size=BUFFER_SIZE,i_size,o_size,r_size;
      short int buffer_w[BUFFER_SIZE*2];
      float buffer_fm[BUFFER_SIZE],buffer_fs[BUFFER_SIZE*2],*buffer_f;
      int i,mtype,mflags,tracks;
      //int ival=0,oval,vs=10;
      const char *type_name[3]={"","generator","effect"};
      int nan=0,inf=0,den=0;
      int clipped=0;
      float ma=0.0;
      int mode=3/*WM_READWRITE*/;
      int triggered=TRUE;

      puts("  windows machine created");
      bml(init(bm,0,NULL));
      bml(get_machine_info(bmh,BM_PROP_TYPE,&mtype));
      bml(get_machine_info(bmh,BM_PROP_FLAGS,&mflags));
      printf("  %s initialized\n",type_name[mtype]);
      bml(get_machine_info(bmh,BM_PROP_MIN_TRACKS,(void *)&tracks));
      if(tracks) {
        bml(set_num_tracks(bm,tracks));
        printf("  activated %d tracks\n",tracks);
      }

      //bml(stop(bm));
      //bml(attributes_changed(bm));

      // open raw files
      infile=fopen(infilename,"rb");
      outfile=fopen(outfilename,"wb");
      if(infile && outfile) {
        printf("    processing ");
        if(mtype==1) {
          int num,ptype,pflags,pmaval;

          // trigger a note for generators
          bml(get_machine_info(bmh,BM_PROP_NUM_GLOBAL_PARAMS,&num));
          // set value for trigger parameter(s)
          for(i=0;i<num;i++) {
            bml(get_global_parameter_info(bmh,i,BM_PARA_FLAGS,(void *)&pflags));
            if(!(pflags&2)) {
              bml(get_global_parameter_info(bmh,i,BM_PARA_TYPE,(void *)&ptype));
              switch(ptype) {
                case 0: // note
                  bml(set_global_parameter_value(bm,i,32));
                  puts("    triggered global note");
                  break;
                case 1: // switch
                  bml(set_global_parameter_value(bm,i,1));
                  puts("    triggered global switch");
                  break;
                case 2: // byte
                case 3: // word
                  bml(get_global_parameter_info(bmh,i,BM_PARA_NO_VALUE,(void *)&pmaval));
                  bml(set_global_parameter_value(bm,i,pmaval));
                  break;
              }
            }
          }
          if(tracks) {
            bml(get_machine_info(bmh,BM_PROP_NUM_TRACK_PARAMS,&num));
            for(i=0;i<num;i++) {
              bml(get_track_parameter_info(bmh,i,BM_PARA_FLAGS,(void *)&pflags));
              if(!(pflags&2)) {
                bml(get_track_parameter_info(bmh,i,BM_PARA_TYPE,(void *)&ptype));
                switch(ptype) {
                  case 0: // note
                    bml(set_track_parameter_value(bm,0,i,32));
                    puts("    triggered voice note");
                    break;
                  case 1: // switch
                    bml(set_track_parameter_value(bm,0,i,1));
                    puts("    triggered voice switch");
                    break;
                  case 2: // byte
                  case 3: // word
                    bml(get_track_parameter_info(bmh,i,BM_PARA_NO_VALUE,(void *)&pmaval));
                    bml(set_track_parameter_value(bm,0,i,pmaval));
                    break;
                }
              }
            }
          }
          mode=2/*WM_WRITE*/;
        }
        if((mflags&1)==0) { // MIF_MONO_TO_STEREO
          buffer_f=buffer_fm;
        }
        else {
          buffer_f=buffer_fs;
        }
        while(!feof(infile)) {
          // change a parameter
          // assumes the first param is of pt_word type
          //bm_set_global_parameter_value(bm,0,ival);
          //oval=bm_get_global_parameter_value(bm,0);printf("        Value: %d\n",oval);
          //ival+=vs;
          //if(((vs>0) && (ival==1000)) || ((vs<0) && (ival==0))) vs=-vs;

          printf(".");
          // set GlobalVals, TrackVals
          bml(tick(bm));
          i_size=fread(buffer_w,2,s_size,infile);
          if(i_size) {
            // generators get silence, effects get the input
            if(mtype==1) {
              for(i=0;i<i_size;i++) buffer_fm[i]=0.0;
            }
            else {
              for(i=0;i<i_size;i++) buffer_fm[i]=(float)buffer_w[i];
            }
            if((mflags&1)==0) { // MIF_MONO_TO_STEREO
              bml(work(bm,buffer_fm,i_size,mode));
              o_size=i_size;
            }
            else {
              bml(work_m2s(bm,buffer_fm,buffer_fs,i_size,mode));
              o_size=i_size*2;
            }
            for(i=0;i<o_size;i++) {
              if(isnan(buffer_f[i])) nan=1;
              if(isinf(buffer_f[i])) inf=1;
              if(fpclassify(buffer_f[i])==FP_SUBNORMAL) den=1;
              if(fabs(buffer_f[i])>ma) ma=buffer_f[i];
              if(buffer_f[i]>32767) {
                buffer_w[i]=32767;
                clipped++;
              }
              else if(buffer_f[i]<-32768) {
                buffer_w[i]=-32768;
                clipped++;
              }
              else {
                buffer_w[i]=(short int)(buffer_f[i]);
              }
            }
            if((r_size=fwrite(buffer_w,2,o_size,outfile))<o_size)
            	break;
          }

          // reset trigger
          if((mtype==1) && triggered) {
            int num,ptype,pflags,pnoval;

            // trigger a note for generators
            bml(get_machine_info(bmh,BM_PROP_NUM_GLOBAL_PARAMS,&num));
            // set value for trigger parameter(s)
            for(i=0;i<num;i++) {
              bml(get_global_parameter_info(bmh,i,BM_PARA_FLAGS,(void *)&pflags));
              if(!(pflags&2)) {
                bml(get_global_parameter_info(bmh,i,BM_PARA_TYPE,(void *)&ptype));
                switch(ptype) {
                  case 0: // note
                    bml(set_global_parameter_value(bm,i,0));
                    puts("    triggered global note");
                    break;
                  case 1: // switch
                    bml(set_global_parameter_value(bm,i,255));
                    puts("    triggered global switch");
                    break;
                  case 2: // byte
                  case 3: // word
                    bml(get_global_parameter_info(bmh,i,BM_PARA_NO_VALUE,(void *)&pnoval));
                    bml(set_global_parameter_value(bm,i,pnoval));
                    break;
                }
              }
            }
            if(tracks) {
              bml(get_machine_info(bmh,BM_PROP_NUM_TRACK_PARAMS,&num));
              for(i=0;i<num;i++) {
                bml(get_track_parameter_info(bmh,i,BM_PARA_FLAGS,(void *)&pflags));
                if(!(pflags&2)) {
                  bml(get_track_parameter_info(bmh,i,BM_PARA_TYPE,(void *)&ptype));
                  switch(ptype) {
                    case 0: // note
                      bml(set_track_parameter_value(bm,0,i,0));
                      puts("    triggered voice note");
                      break;
                    case 1: // switch
                      bml(set_track_parameter_value(bm,0,i,255));
                      puts("    triggered voice switch");
                      break;
                    case 2: // byte
                    case 3: // word
                      bml(get_track_parameter_info(bmh,i,BM_PARA_NO_VALUE,(void *)&pnoval));
                      bml(set_track_parameter_value(bm,0,i,pnoval));
                      break;
                  }
                }
              }
            }
            triggered=FALSE;
          }
        }
        //printf("\n");
      }
      else puts("    file error!");
      bml(free(bm));

      if(infile) fclose(infile);
      if(outfile) fclose(outfile);
      puts("  done");
      if(nan) puts("some values are nan");
      if(inf) puts("some values are inf");
      if(den) puts("some values are denormal");
      printf("Clipped: %d\n",clipped);
      printf("MaxAmp: %f\n",ma);
      okay=1;
    }
    bml(close(bmh));
  }
  return okay;
}

