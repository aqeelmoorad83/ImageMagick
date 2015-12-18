/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                  TTTTT  H   H  RRRR   EEEEE   AAA   DDDD                    %
%                    T    H   H  R   R  E      A   A  D   D                   %
%                    T    HHHHH  RRRR   EEE    AAAAA  D   D                   %
%                    T    H   H  R R    E      A   A  D   D                   %
%                    T    H   H  R  R   EEEEE  A   A  DDDD                    %
%                                                                             %
%                                                                             %
%                         MagickCore Thread Methods                           %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                               March  2003                                   %
%                                                                             %
%                                                                             %
%  Copyright 1999-2016 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    http://www.imagemagick.org/script/license.php                            %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/memory_.h"
#include "magick/thread_.h"
#include "magick/thread-private.h"

/*
  Typedef declarations.
*/
typedef struct _MagickThreadValue
{
  size_t
    number_threads;

  void
    **values,
    (*destructor)(void *);
} MagickThreadValue;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e M a g i c k T h r e a d K e y                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireMagickThreadKey() creates a thread-specific data key visible to all
%  threads in the process.
%
%  The format of the AcquireMagickThreadKey method is:
%
%      MagickThreadKey AcquireMagickThreadKey(MagickThreadKey *key)
%
%  A description of each parameter follows:
%
%    o key: opaque objects used to locate thread-specific data.
%
%    o destructor: associate an optional destructor with each key value.
%
*/
MagickExport MagickBooleanType AcquireMagickThreadKey(MagickThreadKey *key,
  void (*destructor)(void *))
{
#if defined(MAGICKCORE_THREAD_SUPPORT)
  return(pthread_key_create(key,destructor) == 0 ? MagickTrue : MagickFalse);
#elif defined(MAGICKCORE_HAVE_WINTHREADS)
  *key=TlsAlloc();
  return(*key != TLS_OUT_OF_INDEXES ? MagickTrue : MagickFalse);
#else
  {
    MagickThreadValue
      **keys;

    keys=(MagickThreadValue **) key;
    *keys=(MagickThreadValue *) AcquireQuantumMemory(1,sizeof(*keys));
    if (*keys != (MagickThreadValue *) NULL)
      {
        (*keys)->number_threads=GetOpenMPMaximumThreads();
        (*keys)->values=AcquireQuantumMemory((*keys)->number_threads,
          sizeof(void *));
        if ((*keys)->values == (void *) NULL)
          *keys=RelinquishMagickMemory(*keys);
        else
          (void) memset((*keys)->values,0,(*keys)->number_threads*
            sizeof(void *));
        (*keys)->destructor=destructor;
      }
    return((*keys != (MagickThreadValue *) NULL) ? MagickTrue : MagickFalse);
  }
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y  M a g i c k T h r e a d K e y                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyMagickThreadKey() destroy a thread key.
%
%  The format of the DestroyMagickThreadKey method is:
%
%      MagickBooleanType DestroyMagickThreadKey(MagickThreadKey key)
%
%  A description of each parameter follows:
%
%    o key: the thread key.
%
*/
MagickExport MagickBooleanType DestroyMagickThreadKey(MagickThreadKey key)
{
#if defined(MAGICKCORE_THREAD_SUPPORT)
  return(pthread_key_delete(key) == 0 ? MagickTrue : MagickFalse);
#elif defined(MAGICKCORE_HAVE_WINTHREADS)
  return(TlsFree(key) != 0 ? MagickTrue : MagickFalse);
#else
  {
    MagickThreadValue
      *keys;

    register ssize_t
      i;

    keys=(MagickThreadValue *) key;
    for (i=0; i < (ssize_t) keys->number_threads; i++)
      if (keys->values[i] != (void *) NULL)
        {
          keys->destructor(keys->values[i]);
          keys->values[i]=(void *) NULL;
        }
    keys=(MagickThreadValue *) RelinquishMagickMemory(keys);
  }
  return(MagickTrue);
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k T h r e a d V a l u e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickThreadKey() returns a value associated with the thread key.
%
%  The format of the GetMagickThreadKey method is:
%
%      void *GetMagickThreadKey(MagickThreadKey key)
%
%  A description of each parameter follows:
%
%    o key: the thread key.
%
*/
MagickExport void *GetMagickThreadKey(MagickThreadKey key)
{
#if defined(MAGICKCORE_THREAD_SUPPORT)
  return(pthread_getspecific(key));
#elif defined(MAGICKCORE_HAVE_WINTHREADS)
  return(TlsGetValue(key));
#else
  {
    MagickThreadValue
      *keys;

    keys=(MagickThreadValue *) key;
    return(keys->values[GetOpenMPThreadId()]);
  }
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t M a g i c k T h r e a d V a l u e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetMagickThreadKey() associates a value with the thread key.
%
%  The format of the SetMagickThreadKey method is:
%
%      MagickBooleanType SetMagickThreadKey(MagickThreadKey key,
%        const void *value)
%
%  A description of each parameter follows:
%
%    o key: the thread key.
%
%    o value: the value.
%
*/
MagickExport MagickBooleanType SetMagickThreadKey(MagickThreadKey key,
  const void *value)
{
#if defined(MAGICKCORE_THREAD_SUPPORT)
  return(pthread_setspecific(key,value) == 0 ? MagickTrue : MagickFalse);
#elif defined(MAGICKCORE_HAVE_WINTHREADS)
  return(TlsSetValue(key,(void *) value) != 0 ? MagickTrue : MagickFalse);
#else
  {
    MagickThreadValue
      *keys;

    keys=(MagickThreadValue *) key;
    keys->values[GetOpenMPThreadId()]=(void *) value;
  }
  return(MagickTrue);
#endif
}