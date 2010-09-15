//  Copyright (C) 2010 Ultr@VNC Team Members. All Rights Reserved.
// 
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


#include "communication.h"


comm_serv::comm_serv()
{
	Force_unblock();
	event_E_IN=NULL;
	event_E_IN_DONE=NULL;
	event_E_OUT=NULL;
	event_E_OUT_DONE=NULL;
	data_IN=NULL;
	data_OUT=NULL;
	hMapFile_IN=NULL;
	hMapFile_OUT=NULL;
	InitializeCriticalSection(&CriticalSection_IN); 
	InitializeCriticalSection(&CriticalSection_OUT); 
	timeout_counter=0;
}

comm_serv::~comm_serv()
{
	CloseHandle(event_E_IN);
	CloseHandle(event_E_IN_DONE);
	CloseHandle(event_E_OUT);
	CloseHandle(event_E_OUT_DONE);
	if (data_IN)UnmapViewOfFile(data_IN);
	if (data_OUT)UnmapViewOfFile(data_OUT);
	if (hMapFile_IN)CloseHandle(hMapFile_IN);
	if (hMapFile_OUT)CloseHandle(hMapFile_OUT);
	DeleteCriticalSection(&CriticalSection_IN);
	DeleteCriticalSection(&CriticalSection_OUT);
}


void 
	comm_serv::create_sec_attribute()
{
		char secDesc[ SECURITY_DESCRIPTOR_MIN_LENGTH ];
		secAttr.nLength = sizeof(secAttr);
		secAttr.bInheritHandle = FALSE;
		secAttr.lpSecurityDescriptor = &secDesc;
		InitializeSecurityDescriptor(secAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(secAttr.lpSecurityDescriptor, TRUE, 0, FALSE);
		TCHAR * szSD = TEXT("D:")       // Discretionary ACL
			//TEXT("(D;OICI;GA;;;BG)")     // Deny access to built-in guests
			//TEXT("(D;OICI;GA;;;AN)")     // Deny access to anonymous logon
			TEXT("(A;OICI;GRGWGX;;;AU)") // Allow read/write/execute to authenticated users
			TEXT("(A;OICI;GA;;;BA)");    // Allow full control to administrators

		PSECURITY_DESCRIPTOR pSD;
		BOOL retcode =ConvertStringSecurityDescriptorToSecurityDescriptor("S:(ML;;NW;;;LW)",SDDL_REVISION_1,&pSD,NULL);
		DWORD aa=GetLastError();

		if(retcode != 0){ 
		PACL pSacl = NULL;
		BOOL fSaclPresent = FALSE;
		BOOL fSaclDefaulted = FALSE;
		retcode =GetSecurityDescriptorSacl(
			pSD,
			&fSaclPresent,
			&pSacl,
			&fSaclDefaulted);
		if (pSacl) retcode =SetSecurityDescriptorSacl(secAttr.lpSecurityDescriptor, TRUE, pSacl, FALSE); 
		}
}

bool comm_serv::Init(char *name,int IN_datasize_IN,int IN_datasize_OUT,bool app,bool master)
{
	datasize_IN=IN_datasize_IN;
	datasize_OUT=IN_datasize_OUT;

	char secDesc[ SECURITY_DESCRIPTOR_MIN_LENGTH ];
		secAttr.nLength = sizeof(secAttr);
		secAttr.bInheritHandle = FALSE;
		secAttr.lpSecurityDescriptor = &secDesc;
		InitializeSecurityDescriptor(secAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(secAttr.lpSecurityDescriptor, TRUE, 0, FALSE);
		TCHAR * szSD = TEXT("D:")       // Discretionary ACL
			//TEXT("(D;OICI;GA;;;BG)")     // Deny access to built-in guests
			//TEXT("(D;OICI;GA;;;AN)")     // Deny access to anonymous logon
			TEXT("(A;OICI;GRGWGX;;;AU)") // Allow read/write/execute to authenticated users
			TEXT("(A;OICI;GA;;;BA)");    // Allow full control to administrators

		PSECURITY_DESCRIPTOR pSD;
		BOOL retcode =ConvertStringSecurityDescriptorToSecurityDescriptor("S:(ML;;NW;;;LW)",SDDL_REVISION_1,&pSD,NULL);
		DWORD aa=GetLastError();

		if(retcode != 0){ 
		PACL pSacl = NULL;
		BOOL fSaclPresent = FALSE;
		BOOL fSaclDefaulted = FALSE;
		retcode =GetSecurityDescriptorSacl(
			pSD,
			&fSaclPresent,
			&pSacl,
			&fSaclDefaulted);
		if (pSacl) retcode =SetSecurityDescriptorSacl(secAttr.lpSecurityDescriptor, TRUE, pSacl, FALSE); 
		}

	char savename[42];
	strcpy_s(savename,42,name);
	if (app)
	{
		strcpy_s(filemapping_IN,64,"");
		strcpy_s(filemapping_OUT,64,"");
		strcpy_s(event_IN,64,"");
		strcpy_s(event_IN_DONE,64,"");
		strcpy_s(event_OUT,64,"");
		strcpy_s(event_OUT_DONE,64,"");

		strcat_s(filemapping_IN,64,name);
		strcat_s(filemapping_IN,64,"fm_IN");
		strcat_s(filemapping_OUT,64,name);
		strcat_s(filemapping_OUT,64,"fm_OUT");
		strcat_s(event_IN,64,name);
		strcat_s(event_IN,64,"event_IN");
		strcat_s(event_IN_DONE,64,name);
		strcat_s(event_IN_DONE,64,"event_IN_DONE");
		strcat_s(event_OUT,64,name);
		strcat_s(event_OUT,64,"event_OUT");
		strcat_s(event_OUT_DONE,64,name);
		strcat_s(event_OUT_DONE,64,"event_OUT_DONE");
	}
	else
	{
		strcpy_s(filemapping_IN,64,"Global\\");
		strcpy_s(filemapping_OUT,64,"Global\\");
		strcpy_s(event_IN,64,"Global\\");
		strcpy_s(event_IN_DONE,64,"Global\\");
		strcpy_s(event_OUT,64,"Global\\");
		strcpy_s(event_OUT_DONE,64,"Global\\");

		strcat_s(filemapping_IN,64,name);
		strcat_s(filemapping_IN,64,"fm_IN");
		strcat_s(filemapping_OUT,64,name);
		strcat_s(filemapping_OUT,64,"fm_OUT");
		strcat_s(event_IN,64,name);
		strcat_s(event_IN,64,"event_IN");
		strcat_s(event_IN_DONE,64,name);
		strcat_s(event_IN_DONE,64,"event_IN_DONE");
		strcat_s(event_OUT,64,name);
		strcat_s(event_OUT,64,"event_OUT");
		strcat_s(event_OUT_DONE,64,name);
		strcat_s(event_OUT_DONE,64,"event_OUT_DONE");
	}

	if (master)
	{
	if (!app)
	{
		if (datasize_IN!=0)
		{
		hMapFile_IN = CreateFileMapping(INVALID_HANDLE_VALUE,&secAttr,PAGE_READWRITE,0,datasize_IN,filemapping_IN);
		if (hMapFile_IN == NULL) return false;
		data_IN=(char*)MapViewOfFile(hMapFile_IN,FILE_MAP_ALL_ACCESS,0,0,datasize_IN);           
		if(data_IN==NULL) return false;
		}
		event_E_IN=CreateEvent(&secAttr, FALSE, FALSE, event_IN);
		if(event_E_IN==NULL) return false;
		event_E_IN_DONE=CreateEvent(&secAttr, FALSE, FALSE, event_IN_DONE);
		if(event_IN_DONE==NULL) return false;

		if (datasize_OUT!=0)
		{
		hMapFile_OUT = CreateFileMapping(INVALID_HANDLE_VALUE,&secAttr,PAGE_READWRITE,0,datasize_OUT,filemapping_OUT);
		if (hMapFile_OUT == NULL) return false;
		data_OUT=(char*)MapViewOfFile(hMapFile_OUT,FILE_MAP_ALL_ACCESS,0,0,datasize_OUT);           
		if(data_OUT==NULL) return false;
		}
		event_E_OUT=CreateEvent(&secAttr, FALSE, FALSE, event_OUT);
		if(event_E_OUT==NULL) return false;
		event_E_OUT_DONE=CreateEvent(&secAttr, FALSE, FALSE, event_OUT_DONE);
		if(event_OUT_DONE==NULL) return false;
	}
	else
	{
		if (datasize_IN!=0)
		{
		hMapFile_IN = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,datasize_IN,filemapping_IN);
		if (hMapFile_IN == NULL) return false;
		data_IN=(char*)MapViewOfFile(hMapFile_IN,FILE_MAP_ALL_ACCESS,0,0,datasize_IN);           
		if(data_IN==NULL) return false;
		}
		event_E_IN=CreateEvent(NULL, FALSE, FALSE, event_IN);
		if(event_E_IN==NULL) return false;
		event_E_IN_DONE=CreateEvent(NULL, FALSE, FALSE, event_IN_DONE);
		if(event_IN_DONE==NULL) return false;

		if (datasize_OUT!=0)
		{
		hMapFile_OUT = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,datasize_OUT,filemapping_OUT);
		if (hMapFile_OUT == NULL) return false;
		data_OUT=(char*)MapViewOfFile(hMapFile_OUT,FILE_MAP_ALL_ACCESS,0,0,datasize_OUT);           
		if(data_OUT==NULL) return false;
		}
		event_E_OUT=CreateEvent(NULL, FALSE, FALSE, event_OUT);
		if(event_E_OUT==NULL) return false;
		event_E_OUT_DONE=CreateEvent(NULL, FALSE, FALSE, event_OUT_DONE);
		if(event_OUT_DONE==NULL) return false;
	}
	}
	else
	{
		if (!app)
		{
			if (datasize_IN!=0)
			{
			hMapFile_IN = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,filemapping_IN);
			DWORD aa=GetLastError();
			if (hMapFile_IN == NULL) return false;
			data_IN=(char*)MapViewOfFile(hMapFile_IN,FILE_MAP_ALL_ACCESS,0,0,datasize_IN);           
			if(data_IN==NULL) return false;
			}
			event_E_IN=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_IN);
			if(event_E_IN==NULL) return false;
			ResetEvent(event_E_IN);
			event_E_IN_DONE=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_IN_DONE);
			if(event_IN_DONE==NULL) return false;
			ResetEvent(event_IN_DONE);
			if (datasize_OUT!=0)
			{
			hMapFile_OUT = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,filemapping_OUT);
			if (hMapFile_OUT == NULL) return false;
			data_OUT=(char*)MapViewOfFile(hMapFile_OUT,FILE_MAP_ALL_ACCESS,0,0,datasize_OUT);           
			if(data_OUT==NULL) return false;
			}
			event_E_OUT=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_OUT);
			if(event_E_OUT==NULL) return false;
			ResetEvent(event_E_OUT);
			event_E_OUT_DONE=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_OUT_DONE);
			if(event_OUT_DONE==NULL) return false;
			ResetEvent(event_OUT_DONE);
		}
		else
		{
			if (datasize_IN!=0)
			{
			hMapFile_IN = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,filemapping_IN);
			if (hMapFile_IN == NULL) return false;
			data_IN=(char*)MapViewOfFile(hMapFile_IN,FILE_MAP_ALL_ACCESS,0,0,datasize_IN);           
			if(data_IN==NULL) return false;
			}
			event_E_IN=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_IN);
			if(event_E_IN==NULL) return false;
			ResetEvent(event_E_IN);
			event_E_IN_DONE=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_IN_DONE);
			if(event_IN_DONE==NULL) return false;
			ResetEvent(event_IN_DONE);

			if (datasize_OUT!=0)
			{
			hMapFile_OUT =OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,filemapping_OUT);
			if (hMapFile_OUT == NULL) return false;
			data_OUT=(char*)MapViewOfFile(hMapFile_OUT,FILE_MAP_ALL_ACCESS,0,0,datasize_OUT);           
			if(data_OUT==NULL) return false;
			}
			event_E_OUT=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_OUT);
			if(event_E_OUT==NULL) return false;
			ResetEvent(event_E_OUT);
			event_E_OUT_DONE=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_OUT_DONE);
			if(event_OUT_DONE==NULL) return false;
			ResetEvent(event_OUT_DONE);
		}
	}
	return true;
}


HANDLE comm_serv::InitFileHandle(char *name,int IN_datasize_IN,int IN_datasize_OUT,bool app,bool master)
{
	datasize_IN=IN_datasize_IN;
	datasize_OUT=IN_datasize_OUT;
	create_sec_attribute();
	char savename[42];
	strcpy_s(savename,42,name);
	if (app)
	{
		strcpy_s(filemapping_IN,64,"");
		strcpy_s(filemapping_OUT,64,"");
		strcpy_s(event_IN,64,"");
		strcpy_s(event_IN_DONE,64,"");
		strcpy_s(event_OUT,64,"");
		strcpy_s(event_OUT_DONE,64,"");

		strcat_s(filemapping_IN,64,name);
		strcat_s(filemapping_IN,64,"fm_IN");
		strcat_s(filemapping_OUT,64,name);
		strcat_s(filemapping_OUT,64,"fm_OUT");
		strcat_s(event_IN,64,name);
		strcat_s(event_IN,64,"event_IN");
		strcat_s(event_IN_DONE,64,name);
		strcat_s(event_IN_DONE,64,"event_IN_DONE");
		strcat_s(event_OUT,64,name);
		strcat_s(event_OUT,64,"event_OUT");
		strcat_s(event_OUT_DONE,64,name);
		strcat_s(event_OUT_DONE,64,"event_OUT_DONE");
	}
	else
	{
		strcpy_s(filemapping_IN,64,"Global\\");
		strcpy_s(filemapping_OUT,64,"Global\\");
		strcpy_s(event_IN,64,"Global\\");
		strcpy_s(event_IN_DONE,64,"Global\\");
		strcpy_s(event_OUT,64,"Global\\");
		strcpy_s(event_OUT_DONE,64,"Global\\");

		strcat_s(filemapping_IN,64,name);
		strcat_s(filemapping_IN,64,"fm_IN");
		strcat_s(filemapping_OUT,64,name);
		strcat_s(filemapping_OUT,64,"fm_OUT");
		strcat_s(event_IN,64,name);
		strcat_s(event_IN,64,"event_IN");
		strcat_s(event_IN_DONE,64,name);
		strcat_s(event_IN_DONE,64,"event_IN_DONE");
		strcat_s(event_OUT,64,name);
		strcat_s(event_OUT,64,"event_OUT");
		strcat_s(event_OUT_DONE,64,name);
		strcat_s(event_OUT_DONE,64,"event_OUT_DONE");
	}

	if (master)
	{
	if (!app)
	{
		if (datasize_IN!=0)
		{
		hMapFile_IN = CreateFileMapping(INVALID_HANDLE_VALUE,&secAttr,PAGE_READWRITE,0,datasize_IN,filemapping_IN);
		if (hMapFile_IN == NULL) return NULL;
		}
	}
	else
	{
		if (datasize_IN!=0)
		{
		hMapFile_IN = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,datasize_IN,filemapping_IN);
		if (hMapFile_IN == NULL) return NULL;
		}
	}
	}
	else
	{
		if (!app)
		{
			if (datasize_IN!=0)
			{
			hMapFile_IN = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,filemapping_IN);
			if (hMapFile_IN == NULL) return NULL;
			}
		}
		else
		{
			if (datasize_IN!=0)
			{
			hMapFile_IN = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,filemapping_IN);
			if (hMapFile_IN == NULL) return NULL;
			}
		}
	}
	return hMapFile_IN;
}

//service call session function
void comm_serv::Call_Fnction(char *databuffer_IN,char *databuffer_OUT)
{
#ifdef _DEBUG
		char			szText[256];
					sprintf(szText," ++++++ Call_Fnction %s  \n",filemapping_IN);
					SetLastError(0);
					OutputDebugString(szText);		
#endif
	if (!GLOBAL_RUNNING) return;
	EnterCriticalSection(&CriticalSection_IN);
	memcpy(data_IN,databuffer_IN,datasize_IN);
	//ResetEvent(event_E_IN_DONE);
	ResetEvent(event_E_OUT);
	ResetEvent(event_E_OUT_DONE);
	SetEvent(event_E_IN);
	DWORD r=WaitForSingleObject(event_E_IN_DONE,1000);

	if (r==WAIT_TIMEOUT) 
	{
		vnclog.Print(LL_INTWARN, "++++++ Call_Fnction timout 1 %s %i \n",filemapping_IN,timeout_counter);
#ifdef _DEBUG
		char			szText[256];
					sprintf(szText," ++++++ Call_Fnction timout 1 %s %i \n",filemapping_IN,timeout_counter);
					SetLastError(0);
					OutputDebugString(szText);		
#endif
		r=1;
		timeout_counter++;
	}
	else
	{
		vnclog.Print(LL_INTWARN, "++++++ Call_Fnction OK 1 %s \n",filemapping_IN);
		timeout_counter=0;
	}

	if (timeout_counter>3)
	{
		GLOBAL_RUNNING=false;
	}
	if (!GLOBAL_RUNNING) goto error;
	r=WaitForSingleObject(event_E_OUT,1000);
	if (r==WAIT_TIMEOUT) 
	{
		vnclog.Print(LL_INTWARN, "a++++++ Call_Fnction timout 2 %s \n",filemapping_IN);
#ifdef _DEBUG
		char			szText[256];
					sprintf(szText," ++++++ Call_Fnction timeout 2 %s\n",filemapping_IN);
					SetLastError(0);
					OutputDebugString(szText);		
#endif
		r=1;
	}
	else vnclog.Print(LL_INTWARN, "++++++ Call_Fnction ok 2 %s \n",filemapping_IN);

	memcpy(databuffer_OUT,data_OUT,datasize_OUT);
	error:
	SetEvent(event_E_OUT_DONE);
#ifdef _DEBUG
//		char			szText[256];
					sprintf(szText," ++++++ event_E_OUT_DONE %s\n",filemapping_IN);
					SetLastError(0);
					OutputDebugString(szText);		
#endif
	LeaveCriticalSection(&CriticalSection_IN);
}

void comm_serv::Call_Fnction_no_feedback()
{
#ifdef _DEBUG
		char			szText[256];
					sprintf(szText," ++++++ call_no feed %s\n",filemapping_IN);
					SetLastError(0);
					OutputDebugString(szText);		
#endif
	SetEvent(event_E_IN);
}

void comm_serv::Call_Fnction_no_feedback_data(char *databuffer_IN,char *databuffer_OUT)
{
#ifdef _DEBUG
		char			szText[256];
					sprintf(szText," ++++++ Call_Fnction_no_feedback_data %s\n",filemapping_IN);
					SetLastError(0);
					OutputDebugString(szText);		
#endif
	EnterCriticalSection(&CriticalSection_IN);
	memcpy(data_IN,databuffer_IN,datasize_IN);
	SetEvent(event_E_IN);
	LeaveCriticalSection(&CriticalSection_IN);
}

//service call session function
void comm_serv::Call_Fnction_Long(char *databuffer_IN,char *databuffer_OUT)
{
	EnterCriticalSection(&CriticalSection_IN);
	memcpy(data_IN,databuffer_IN,datasize_IN);
	SetEvent(event_E_IN);
	DWORD r=WaitForSingleObject(event_E_IN_DONE,10000);
	if (r==WAIT_TIMEOUT) 
		r=1;
		
	LeaveCriticalSection(&CriticalSection_IN);

	EnterCriticalSection(&CriticalSection_OUT);
	r=WaitForSingleObject(event_E_OUT,10000);
	if (r==WAIT_TIMEOUT) 
		r=1;

	memcpy(databuffer_OUT,data_OUT,datasize_OUT);
	SetEvent(event_E_OUT_DONE);
	LeaveCriticalSection(&CriticalSection_OUT);
}

//service call session function
void comm_serv::Call_Fnction_Long_Timeout(char *databuffer_IN,char *databuffer_OUT,int timeout)
{
	timeout=(timeout+1)*1000;
	EnterCriticalSection(&CriticalSection_IN);
	memcpy(data_IN,databuffer_IN,datasize_IN);
	ResetEvent(event_E_IN_DONE);
	ResetEvent(event_E_OUT);
	SetEvent(event_E_IN);
	DWORD r=WaitForSingleObject(event_E_IN_DONE,timeout);		
	LeaveCriticalSection(&CriticalSection_IN);
	if (r==WAIT_TIMEOUT) 
	{
		unsigned char value=99;
		memcpy(databuffer_OUT,&value,datasize_OUT);
		return;
	}

	EnterCriticalSection(&CriticalSection_OUT);
	r=WaitForSingleObject(event_E_OUT,timeout);
	memcpy(databuffer_OUT,data_OUT,datasize_OUT);
	SetEvent(event_E_OUT_DONE);
	if (r==WAIT_TIMEOUT) 
	{
		unsigned char value=99;
		memcpy(databuffer_OUT,&value,datasize_OUT);
		return;
	}
	LeaveCriticalSection(&CriticalSection_OUT);
}

HANDLE comm_serv::GetEvent()
{
	return event_E_IN;
}

char *comm_serv::Getsharedmem()
{
	return data_IN;
}

void comm_serv::ReadData(char *databuffer)
{
	memcpy(databuffer,data_IN,datasize_IN);
	SetEvent(event_E_IN_DONE);
}

void comm_serv::SetData(char *databuffer)
{
	if (!GLOBAL_RUNNING) return;
	memcpy(data_OUT,databuffer,datasize_OUT);
	SetEvent(event_E_OUT);
	DWORD r=WaitForSingleObject(event_E_OUT_DONE,5);
	if (r==WAIT_TIMEOUT) 
	{
		vnclog.Print(LL_INTWARN, "++++++ SetData timeout  %s \n",filemapping_IN);
		r=1;
	}
	else vnclog.Print(LL_INTWARN, "++++++ SetData ok  %s \n",filemapping_IN);
}

void comm_serv::Force_unblock()
{
	SetEvent(event_E_OUT_DONE);
	SetEvent(event_E_IN_DONE);
	SetEvent(event_E_OUT);
}

void comm_serv::Release()
{
#ifdef _DEBUG
		char			szText[256];
					sprintf(szText," ++++++ call_release %s\n",filemapping_IN);
					SetLastError(0);
					OutputDebugString(szText);		
#endif
	ResetEvent(event_E_IN);
}

