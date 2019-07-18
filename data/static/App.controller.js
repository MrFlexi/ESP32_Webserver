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
		
		onInit: function() {

			var namespace = '';
			var ws =  new WebSocket("ws://192.168.43.34/ws");
			//var ws = new WebSocket("ws://192.168.43.34/ws");

			var oView = this.getView();

			// Dynamisches Menü						
			var MenuModel = new JSONModel("./static/menu.json");
			oView.setModel(MenuModel);
			
			
			this.getView().setModel(oModelLokList, "LokListModel");
			this.getView().setModel(oModelUserList, "oModelUserList");
			this.getView().setModel(oModelMainController, "oModelMainController");


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