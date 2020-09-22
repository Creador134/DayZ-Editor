

class EditorScriptView: ScriptView
{
	protected Editor m_Editor;
	protected EditorHud m_EditorHud;
	
	void EditorScriptView(ScriptView parent = null)
	{
		EditorLog.Trace("EditorScriptView");
		m_Editor = GetEditor();
		if (m_Editor) {
			m_EditorHud = m_Editor.GetEditorHud();	
		}
			
	}
	
	void ~EditorScriptView()
	{
		EditorLog.Trace("~EditorScriptView");
	}
	
	void SetPosition(float x, float y) {
		m_LayoutRoot.SetPos(x, y);
	}
}

