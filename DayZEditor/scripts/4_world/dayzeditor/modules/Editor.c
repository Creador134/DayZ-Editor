
private ref EditorClientModule m_EditorInstance;
EditorClientModule GetEditor() 
{ 	
	//if (m_EditorInstance == null)
	//	m_EditorInstance = new Editor();
	return m_EditorInstance;
}

bool IsEditor() { return m_EditorInstance != null; }


enum EditorClientModuleRPC
{
	INVALID = 36114,
	COUNT
};


class EditorClientModule: JMModuleBase
{
	/* Private Members */
	private Mission m_Mission;
	private UIManager m_UIManager;
	private EditorCamera m_Camera;
	private PlayerBase m_Player;

	
	static Object							ObjectUnderCursor = null;
	static EditorObject 					EditorObjectUnderCursor = null;
	static vector 							CurrentMousePosition;
	
	private bool m_Active = false;

	/* UI Stuff */
	EditorUI GetEditorUI() { return m_EditorUI; }
	EditorCamera GetCamera() { return m_Camera; }
	EditorSettings GetSettings() { return GetModuleManager().GetModule(EditorSettings); }
	
	
	bool IsPlacing() { return m_ObjectInHand != null; }
	
	
	private ref EditorHologram 				m_ObjectInHand;
	private ref EditorUI 					m_EditorUI;
	private ref EditorBrush					m_EditorBrush;
	
	private ref EditorPlaceableListItemSet 		m_PlaceableObjects;
	private ref EditorObjectSet 				m_PlacedObjects;
	private ref EditorObjectSet					m_SelectedObjects;
	private ref EditorObjectDataSet			 	m_SessionCache;
	private ref set<ref EditorAction> 			m_ActionStack;
	
	ref set<ref EditorAction> GetActionStack() { return m_ActionStack; }
	ref EditorObjectSet GetSelectedObjects() { return m_SelectedObjects; }
	ref EditorObjectSet GetPlacedObjects() { return m_PlacedObjects; }
	ref EditorObjectDataSet	 GetSessionCache() { return m_SessionCache; }
		
	// Public Members
	
	
	void EditorClientModule()
	{
		EditorLog.Info("Editor");
		m_EditorInstance = this;
	}
	
	void ~EditorClientModule()
	{
		EditorLog.Info("~Editor");
	}
	
	
	// JMModuleBase Overrides
	override void OnInit()
	{
		EditorLog.Trace("Editor::OnInit");
		m_SessionCache = new EditorObjectDataSet();
		
		// Init UI
		m_UIManager = GetGame().GetUIManager();
		m_EditorUI = new EditorUI();
		m_UIManager.ShowScriptedMenu(m_EditorUI, m_UIManager.GetMenu());
		
		// Load PlaceableObjects
		/*
		m_PlaceableObjects = new EditorPlaceableListItemSet();
		EditorLog.Info(string.Format("Loaded %1 Placeable Objects", EditorSettings.GetPlaceableObjects(m_PlaceableObjects)));
		foreach (ref EditorPlaceableListItem placeable_object: m_PlaceableObjects) {
			m_EditorUI.InsertPlaceableObject(placeable_object);
		}*/
		
		// Events
		EditorEvents.OnObjectCreated.Insert(OnObjectCreated);
		EditorEvents.OnObjectDeleted.Insert(OnObjectDeleted);
		
		// Keybinds
		RegisterBinding(new JMModuleBinding("OnEditorToggleActive", "EditorToggleActive"));
		RegisterBinding(new JMModuleBinding("OnEditorToggleCursor", "EditorToggleCursor"));
		
	}
	
	private bool exit_condition = false;
	override void OnUpdate(float timeslice)
	{
		if (m_Camera != null && !IsMissionOffline()) {
			ScriptRPC update_rpc = new ScriptRPC();
			update_rpc.Write(m_Camera.GetPosition());
			update_rpc.Write(m_Camera.GetOrientation());
			update_rpc.Send(null, EditorServerModuleRPC.EDITOR_CLIENT_UPDATE, true);
		}
		
		
		set<Object> obj = new set<Object>();
		int x, y;
		GetCursorPos(x, y);
		CurrentMousePosition = MousePosToRay(obj);
	
		if (!IsPlacing()) {
			Object target = obj.Get(0);
			if (target != null) {
				if (target != ObjectUnderCursor) {
					if (ObjectUnderCursor != null) OnMouseExitObject(ObjectUnderCursor, x, y);
					OnMouseEnterObject(target, x, y);
					ObjectUnderCursor = target;
				}
				
			} else if (ObjectUnderCursor != null) {
				exit_condition = OnMouseExitObject(ObjectUnderCursor, x, y);
				ObjectUnderCursor = null;
			}
		}
	}
	
	override bool IsServer() { return false; }	

	override void OnInvokeConnect(PlayerBase player, PlayerIdentity identity)
	{
		EditorLog.Trace("Editor::OnInvokeConnect");		
		m_Player = player;
		EditorLog.Debug(m_Player);
	}
		
	override void OnMissionStart()
	{
		EditorLog.Trace("Editor::OnMissionStart");
		m_Mission = GetGame().GetMission();
		
		// Only runs in Singleplayer
		if (IsMissionOffline()) {
			EditorLog.Info("Loading Offline Editor...");
			
			// Random cam position smile :)
			float x = Math.RandomInt(3500, 8500);
			float z = Math.RandomInt(3500, 8500);
			float y = GetGame().SurfaceY(x, z);
			m_Player = CreateDefaultCharacter(Vector(x, y, z));
			m_Active = true;
			SetActive(m_Active);
		} else {
			ScriptRPC rpc = new ScriptRPC();
			rpc.Send(null, EditorServerModuleRPC.EDITOR_CLIENT_CREATED, true);
		}
	}
	
	override void OnMissionFinish()
	{
		EditorLog.Trace("Editor::OnMissionFinish");
		if (!IsMissionOffline()) {
			ScriptRPC rpc = new ScriptRPC();
			rpc.Send(null, EditorServerModuleRPC.EDITOR_CLIENT_DESTROYED, true);
		}
	}
	
	override void OnMissionLoaded()
	{
		EditorLog.Trace("Editor::OnMissionLoaded");
	}
	
	
	// Inputs
	private void OnEditorToggleActive(UAInput input)
	{
		if (!input.LocalPress()) return; 
		EditorLog.Trace("Editor::OnEditorToggleActive");
		m_Active = !m_Active;
		SetActive(m_Active);
	}	
	
	private void OnEditorToggleCursor(UAInput input)
	{
		if (!input.LocalPress()) return; 
		EditorLog.Trace("Editor::OnEditorToggleCursor");
		EditorUI.ToggleCursor();
	}
		
	
	// RPC stuff
	override int GetRPCMin()
	{
		return EditorClientModuleRPC.INVALID;
	}

	override int GetRPCMax()
	{
		return EditorClientModuleRPC.COUNT;
	}
	
	override void OnRPC(PlayerIdentity sender, Object target, int rpc_type, ref ParamsReadContext ctx)
	{
		switch (rpc_type) {
			
		
		}
	}
	
	// Object Management
	void CreateObjects(ref EditorObjectDataSet data_list, bool create_undo = true)
	{
		EditorLog.Trace("Editor::CreateObjects");

		EditorAction action = new EditorAction("Delete", "Create");
		foreach (EditorObjectData editor_object_data: data_list) {
			
			EditorObject editor_object = new EditorObject(editor_object_data);
			action.InsertUndoParameter(editor_object, new Param1<int>(editor_object.GetID()));
			action.InsertRedoParameter(editor_object, new Param1<int>(editor_object.GetID()));			
		}
		
		if (create_undo) {
			InsertAction(action);
		} else {
			delete action;
		}
		
		EditorEvents.ObjectCreated(this, editor_object);
	}
	
	
	EditorObject CreateObject(ref EditorObjectData editor_object_data, bool create_undo = true)
	{		
		EditorLog.Trace("Editor::CreateObject");
		
		EditorObject editor_object = new EditorObject(editor_object_data);
		EditorEvents.ObjectCreated(this, editor_object);
		
		if (!create_undo) return editor_object;

		
		EditorAction action = new EditorAction("Delete", "Create");;
		action.InsertUndoParameter(editor_object, new Param1<int>(editor_object.GetID()));
		action.InsertRedoParameter(editor_object, new Param1<int>(editor_object.GetID()));
		InsertAction(action);
			
	
		return editor_object;
	}

	void DeleteObject(EditorObject target, bool create_undo = true)
	{
		EditorLog.Trace("Editor::DeleteObject");
		EditorEvents.ObjectDeleted(this, target);
		
		if (create_undo) {
			EditorAction action = new EditorAction("Create", "Delete");
			action.InsertUndoParameter(target, new Param1<int>(target.GetID()));
			action.InsertRedoParameter(target, new Param1<int>(target.GetID()));
			InsertAction(action);
		}
		
		delete target;
	}
	
	void DeleteObjects(EditorObjectSet target, bool create_undo = true)
	{
		EditorLog.Trace("Editor::DeleteObjects");
		
		if (create_undo) EditorAction action = new EditorAction("Create", "Delete");
		
		foreach (EditorObject editor_object: target) {
			EditorEvents.ObjectDeleted(this, editor_object);
			if (create_undo) {
				action.InsertUndoParameter(editor_object, new Param1<int>(editor_object.GetID()));
				action.InsertRedoParameter(editor_object, new Param1<int>(editor_object.GetID()));
			}
			
			delete editor_object;
		}	
			
		if (create_undo) InsertAction(action);
	}
	
	// Call to select an object
	void SelectObject(EditorObject target)
	{
		EditorLog.Trace("Editor::SelectObject");
		m_SelectedObjects.InsertEditorObject(target);
		EditorEvents.ObjectSelected(this, target);
	}
	
	// Call to deselect an object
	void DeselectObject(EditorObject target)
	{
		EditorLog.Trace("Editor::DeselectObject");
		m_SelectedObjects.RemoveEditorObject(target);
		EditorEvents.ObjectDeselected(this, target);
	}	
	
	// Call to toggle selection
	void ToggleSelection(EditorObject target)
	{
		EditorLog.Trace("Editor::ToggleSelection");
		if (target.IsSelected()) {
			DeselectObject(target);
		} else {
			SelectObject(target);
		}
	}
	
	// Call to clear selection
	void ClearSelection()
	{
		Print("Editor::ClearSelection");		
		foreach (EditorObject editor_object: m_SelectedObjects)
			DeselectObject(editor_object);
	}
	
	
	// Call to enable / disable editor
	void SetActive(bool active)
	{
				
		if (m_Camera == null) {

			EditorLog.Info("Initializing Camera");
			
			// Init Spawn Position
			TIntArray center_pos = new TIntArray();		
			string world_name;
			GetGame().GetWorldName(world_name);
			GetGame().ConfigGetIntArray(string.Format("CfgWorlds %1 centerPosition", world_name), center_pos);
			

			
			// Camera Init
			// todo if singleplayer spawn on center of map, otherwise spawn on character in MP
			vector pos = m_Player.GetPosition();
			m_Camera = GetGame().CreateObjectEx("EditorCamera", pos, ECE_LOCAL);
			
			
			// Init Camera Map Marker
			/*
			EditorCameraMapMarker CameraMapMarker = new EditorCameraMapMarker();
			Widget m_MapMarkerWidget = GetGame().GetWorkspace().CreateWidgets("DayZEditor/gui/Layouts/EditorCameraMapMarker.layout");
			m_MapMarkerWidget.GetScript(CameraMapMarker);
			CameraMapMarker.SetCamera(m_Camera, m_EditorUI.GetMapWidget());
			m_EditorUI.InsertMapObject(m_MapMarkerWidget);
			m_EditorUI.GetMapWidget().SetMapPos(Vector(center_pos[0], y_level, center_pos[1]));*/
		}
		
		m_Camera.SetLookEnabled(active);
		m_Camera.SetMoveEnabled(active);
		m_Camera.SetActive(active);
		
		m_EditorUI.Show(active);
		m_Mission.GetHud().Show(!active);
		
		
		if (!active)
			GetGame().SelectPlayer(m_Player.GetIdentity(), m_Player);
	}
	
	bool OnMouseEnterObject(Object target, int x, int y)
	{

	}
	
	bool OnMouseExitObject(Object target, int x, int y)
	{

	}
	
	
	void InsertAction(EditorAction target)
	{
		int count = m_ActionStack.Count();
		for (int i = 0; i < count; i++) {
			if (m_ActionStack[i].IsUndone()) {
				m_ActionStack.Remove(i);
				i--; count--;
			}
		}
			
		// Adds to bottom of stack
		m_ActionStack.InsertAt(target, 0);
		
		// debug
		m_EditorUI.m_DebugActionStack.ClearItems();
		
		for (int debug_i = 0; debug_i < m_ActionStack.Count(); debug_i++) {
			m_EditorUI.m_DebugActionStack.AddItem(m_ActionStack[debug_i].GetName(), m_ActionStack[debug_i], 0);
		}
	}
	

	

	
	void OnObjectCreated(Class context, EditorObject target)
	{
		//m_SelectedObjects.InsertEditorObject(target);
		m_PlacedObjects.InsertEditorObject(target);
		m_PlacedObjectIndex.Insert(target.GetWorldObject().GetID(), target.GetID());
	}	
	
	void OnObjectDeleted(Class context, EditorObject target)
	{
		m_SelectedObjects.RemoveEditorObject(target);
		m_PlacedObjects.RemoveEditorObject(target);
	}
	
	// This is for hashmap lookup of placed objects. very fast
	private ref map<int, int> m_PlacedObjectIndex = new map<int, int>();
	EditorObject GetEditorObject(notnull Object world_object) { return GetEditorObject(m_PlacedObjectIndex.Get(world_object.GetID())); }
	EditorObject GetEditorObject(int id) { return m_PlacedObjects.Get(id); }
	
	void DeleteSessionData(int id) { m_SessionCache.Remove(id);	}
	EditorObjectData GetSessionDataById(int id) { return m_SessionCache.Get(id); }
	EditorObject GetPlacedObjectById(int id) { return m_PlacedObjects.Get(id); }
		
	void SetBrush(EditorBrush brush) { m_EditorBrush = brush; }	
	EditorBrush GetBrush() { return m_EditorBrush; }
	
	
	void CreateInHand(EditorPlaceableObjectData data)
	{
		
		EditorEvents.StartPlacing(this, data);
		
		// todo create hologram in hand
	}
	
	void PlaceObject()
	{
		Input input = GetGame().GetInput();
		EntityAI e = m_ObjectInHand.GetProjectionEntity();
		vector mat[4];
	
		EditorObject editor_object = CreateObject(EditorObjectData.Create(e.GetType(), e.GetPosition(), e.GetOrientation()));
		SelectObject(editor_object);
		
		if (!input.LocalValue("UATurbo")) { 
			delete m_ObjectInHand;
		}
		
		EditorEvents.StopPlacing(this);
	}
	
	
	
	// probably have an EditorMode enum with NORMAL, CHARACTER, LOOTEDITOR or something
	
	private Object m_LootEditTarget;
	private bool m_LootEditMode;
	private vector m_PositionBeforeLootEditMode;
	void PlaceholderForEditLootSpawns(string name)
	{
		m_LootEditTarget = GetGame().CreateObjectEx(name, Vector(0, 1000, 0), ECE_NONE);
		
		m_PositionBeforeLootEditMode = m_Camera.GetPosition();
		m_Camera.SetPosition(Vector(10, 1000, 10));
		//camera.SelectTarget(m_LootEditTarget);
		
		
		ref EditorMapGroupProto proto_data = new EditorMapGroupProto(m_LootEditTarget); 
		EditorXMLManager.LoadMapGroupProto(proto_data);
		m_LootEditMode = true;
		EditorSettings.COLLIDE_ON_DRAG = true;
	}
	
	void PlaceholderRemoveLootMode()
	{
		IEntity child = m_LootEditTarget.GetChildren();
		while (child != null) {
			GetGame().ObjectDelete(Object.Cast(child));
			child = child.GetSibling();
		}
		
		GetGame().ObjectDelete(m_LootEditTarget);
		
		m_Camera.SetPosition(m_PositionBeforeLootEditMode);
		//camera.SelectTarget(null);
		m_LootEditMode = false;
		EditorSettings.COLLIDE_ON_DRAG = false;
	}
	
	bool IsLootEditActive() { return m_LootEditMode; }
	
		
	// todo this is broke
	void OnPlaceableCategoryChanged(Class context, PlaceableObjectCategory category)
	{
		EditorLog.Trace("EditorUIManager::OnPlaceableCategoryChanged");

		foreach (EditorPlaceableListItem placeable_object: m_PlaceableObjects) {
			Widget root = placeable_object.GetRoot();
			root.Show(placeable_object.GetData().GetCategory() == category);
		}
	}
	

}
