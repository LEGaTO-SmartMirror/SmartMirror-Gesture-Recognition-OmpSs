/**
 * @file SmartMirror-Object-Recognition-OmpSs.js
 *
 * @author nkucza
 * @license MIT
 *
 * @see  TODO
 */

Module.register('SmartMirror-Gesture-Recognition-OmpSs',{

	defaults: {
		image_height: 1080,
		image_width: 1920,
		image_stream_path: "/dev/shm/camera_image"
	},

	start: function() {
		this.time_of_last_greeting_personal = [];
		this.time_of_last_greeting = 0;
		this.last_rec_user = [];
		this.current_user = null;
		this.sendSocketNotification('GESTURE_DETECITON_CONFIG', this.config);
		Log.info('Starting module: ' + this.name);
	},

	notificationReceived: function(notification, payload, sender) {
		if(notification === 'smartmirror-gesture-recognitionSetFPS') {
			this.sendSocketNotification('GestureDetection_SetFPS', payload);
        }
	},


	socketNotificationReceived: function(notification, payload) {
		if(notification === 'detected') {
			this.sendNotification('GESTURE_DETECTED', payload);
			//console.log("[" + this.name + "] " + "object detected: " + payload);
        } else if(notification === 'DETECTED_GESTURES') {
			this.sendNotification('DETECTED_GESTURES', payload);
			//console.log("[" + this.name + "] " + "object detected: " + payload);
        }else if (notification === 'GESTURE_DET_FPS') {
			this.sendNotification('GESTURE_DET_FPS', payload);
		};
	}
});
