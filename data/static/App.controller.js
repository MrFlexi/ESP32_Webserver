sap.ui.define([
	'jquery.sap.global',
	'sap/ui/core/Fragment',
	'sap/m/MessageToast',
	'./Formatter',
	'sap/ui/core/mvc/Controller',
	'sap/ui/model/json/JSONModel',
	'sap/m/Popover',
	'sap/m/UploadCollectionParameter',
	'sap/m/Button'
], function (jQuery, Fragment, MessageToast, Formatter, Controller, JSONModel, Popover, Button) {
	"use strict";

	var oModelLokList           = new sap.ui.model.json.JSONModel();
	var oModelUserList          = new sap.ui.model.json.JSONModel();
	var oModelMainController    = new sap.ui.model.json.JSONModel();
	var oModelUser              = new sap.ui.model.json.JSONModel();
	var oModelTerminal           = new sap.ui.model.json.JSONModel();

	//var ws =  new WebSocket("ws://192.168.1.228/ws");
	
	var CController = Controller.extend("view.App", {
		model: new sap.ui.model.json.JSONModel(),
		
		data: {

			navigation: [{
				title: 'Home',
				icon: 'sap-icon://home',
				expanded: true,
				key: 'Home'
			}, {
				title: 'Drive',
				icon: 'sap-icon://cargo-train',
				key: 'Drive',
				expanded: true,
			},
			{
				title: 'MQTT-Messages In',
				icon: 'sap-icon://list',
				expanded: true,
				key: 'lok_list'
			},

			{
				title: 'UMQTT-Messages Out',
				icon: 'sap-icon://account',
				expanded: true,
				key: 'user_list'
			},

			{
				title: 'Devices',
				icon: 'sap-icon://action',
				expanded: false,
				items: [{
					title: 'Show connected'
				}, {
					title: 'Child Item 2'
				}, {
					title: 'Child Item 3'
				}]
			}, ],

			fixedNavigation: [{
				title: 'GPS',
				icon: 'sap-icon://employee'
			}, {
				title: 'Lora Settings',
				icon: 'sap-icon://building'
			}, {
				title: 'Battery Management',
				icon: 'sap-icon://card'
			}],

			headerItems: [{
				text: "File"
			}, {
				text: "Edit"
			}, {
				text: "View"
			}, {
				text: "Settings"
			}, {
				text: "Help"
			}]
		},
		onInit: function() {

			var namespace = '';
			var ws =  new WebSocket("ws://192.168.43.34/ws");
			//var ws = new WebSocket("ws://192.168.43.34/ws");

			var oView = this.getView();
			
			// Dynamisches Menü
			//this.model.setData(this.data);
			//this.getView().setModel(this.model);
			this.getView().setModel(oModelLokList, "LokListModel");
			this.getView().setModel(oModelUserList, "oModelUserList");
			this.getView().setModel(oModelMainController, "oModelMainController");
			//this.getView().setModel(oModelGps, "oModelGps");

			var oData = {
				recipient : {
				   name : "World"
				}
			 };
			var oModel2 = new JSONModel(oData);
         	this.getView().setModel(oModel2,"Test");

			
	
			 ws.onopen = function() {                  
				 // Web Socket is connected, send data using send()			
				//	 alert("WS open im controller");
				 // ws.send("Hallo from Client");
			  };	 
				 
			 ws.onmessage = function (evt)  { 
				 //alert("WS open2 im controller" + evt.data);				
				 var terminal_model = jQuery.parseJSON(evt.data)
				 oModelTerminal.setData(terminal_model);	
				 oView.setModel(oModelTerminal,"Terminal");
						 
			  };
		 

            

		},

		

		onItemSelect: function(oEvent) {
			var item = oEvent.getParameter('item');
			var viewId = this.getView().getId();
			sap.ui.getCore().byId(viewId + "--pageContainer").to(viewId + "--" + item.getKey());
		},

		onSliderliveChange: function(oEvent) {
		   
		},




		handleLocomotionSelectDialogClose: function(oEvent) {
			var aContexts = oEvent.getParameter("selectedContexts");
			if (aContexts && aContexts.length) {
			    var lok_name = aContexts.map(function(oContext) { return oContext.getObject().name; }).join(", ");
			    var lok_id   = aContexts.map(function(oContext) { return oContext.getObject().id; }).join(", ");

				MessageToast.show("You have chosen " + lok_name + lok_id );

		

			}
			oEvent.getSource().getBinding("items").filter([]);
		},

		handleUserSelectDialogClose: function(oEvent) {
			var aContexts = oEvent.getParameter("selectedContexts");
			if (aContexts && aContexts.length) {
			    var user_name = aContexts.map(function(oContext) { return oContext.getObject().user_name; }).join(", ");
			    var user_id   = aContexts.map(function(oContext) { return oContext.getObject().user_id; }).join(", ");

				MessageToast.show("You are logged in as: " + user_name + "  " + user_id );

				

			}
		},



		handleUserNamePress: function(event) {

		},

		onSideNavButtonPress: function() {
			var viewId = this.getView().getId();
			var toolPage = sap.ui.getCore().byId(viewId + "--toolPage");
			var sideExpanded = toolPage.getSideExpanded();

			this._setToggleButtonTooltip(sideExpanded);

			toolPage.setSideExpanded(!toolPage.getSideExpanded());
		},


		_setToggleButtonTooltip: function(bLarge) {
			var toggleButton = this.byId('sideNavigationToggleButton');
			if (bLarge) {
				toggleButton.setTooltip('Large Size Navigation');
			} else {
				toggleButton.setTooltip('Small Size Navigation');
			}
		}

	});

	return CController;

});