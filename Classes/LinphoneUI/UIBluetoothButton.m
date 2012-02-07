/* UISpeakerButton.m
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.                                 
 *                                                                      
 *  This program is distributed in the hope that it will be useful,     
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
 *  GNU General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */      

#import "UIBluetoothButton.h"
#import <AudioToolbox/AudioToolbox.h>
#include "linphonecore.h"

@implementation UIBluetoothButton
#define check_auresult(au,method) \
if (au!=0) ms_error("UIBluetoothButton error for %s: ret=%ld",method,au)

-(void) onOn {
	//redirect audio to bluetooth

	UInt32 size = sizeof(CFStringRef);
	CFStringRef route=CFSTR("HeadsetBT");
	OSStatus result = AudioSessionSetProperty(kAudioSessionProperty_AudioRoute, size, &route);
	check_auresult(result,"set kAudioSessionProperty_AudioRoute HeadsetBT");
	
	int allowBluetoothInput = 1;
	result = AudioSessionSetProperty (
							 kAudioSessionProperty_OverrideCategoryEnableBluetoothInput,
							 sizeof (allowBluetoothInput),
							 &allowBluetoothInput
							 );	
	check_auresult(result,"set kAudioSessionProperty_OverrideCategoryEnableBluetoothInput 1");

}
-(void) onOff {
	//redirect audio to bluetooth
	int allowBluetoothInput = 0;
	OSStatus result =  AudioSessionSetProperty (
							 kAudioSessionProperty_OverrideCategoryEnableBluetoothInput,
							 sizeof (allowBluetoothInput),
							 &allowBluetoothInput
							 );	
	check_auresult(result,"set kAudioSessionProperty_OverrideCategoryEnableBluetoothInput 0");
	UInt32 size = sizeof(CFStringRef);
	CFStringRef route=CFSTR("ReceiverAndMicrophone");
	result = AudioSessionSetProperty(kAudioSessionProperty_AudioRoute, size, &route);
	check_auresult(result,"set kAudioSessionProperty_AudioRoute ReceiverAndMicrophone");

	
}
-(bool) isInitialStateOn {
	return false;
}


/*
 // Only override drawRect: if you perform custom drawing.
 // An empty implementation adversely affects performance during animation.
 - (void)drawRect:(CGRect)rect {
 // Drawing code.
 }
 */

- (void)dealloc {
    [super dealloc];
}


@end
