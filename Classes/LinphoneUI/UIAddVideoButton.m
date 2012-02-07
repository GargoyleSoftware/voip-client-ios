/* UIAddVideoButton.m
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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */       

#import "UIAddVideoButton.h"
#include "LinphoneManager.h"

@implementation UIAddVideoButton

-(void) touchUp:(id) sender {
	LinphoneCore* lc = [LinphoneManager getLc];
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
	if (call) { 
		LinphoneCallParams* call_params =  linphone_call_params_copy(linphone_call_get_current_params(call));
		linphone_call_params_enable_video(call_params, TRUE);
		linphone_core_update_call(lc, call, call_params);
		linphone_call_params_destroy(call_params);
	} {
		ms_warning("Cannot add video, because no current call");
	}
}

- (id) init {
	[self addTarget:self action:@selector(touchUp:) forControlEvents:UIControlEventTouchUpInside];
	return self;
}
- (id)initWithFrame:(CGRect)frame {
    
    self = [super initWithFrame:frame];
    if (self) {
		[self init];
    }
    return self;
}
- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self init];
	}
    return self;
}	

@end
