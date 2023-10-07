using Godot;

public partial class CameraControllerMouse : Node3D
{
    [Export] bool UseLog;
    [Export] float sensitivity;
    Vector2 pos = Vector2.Zero;
	Vector2 delta = Vector2.Zero;
    bool view = false;
    bool reset = false;

    public override void _Ready()
    {
        base._Ready();
    }

    public override void _Input(InputEvent @event)
    {
        base._Input(@event);
        if (@event is InputEventMouseMotion motion && view)
		{
            if(reset == true)
            {
                pos = motion.Position;
                delta = Vector2.Zero;
                reset = false;
            }
            else
            {
                Vector2 newpos = motion.Position;
                delta = (newpos - pos);
                if (UseLog) GD.Print(delta);
                pos = newpos;

                Vector3 rot = Rotation;
                rot.Y += delta.X * sensitivity * 0.001f;
                rot.X += delta.Y * sensitivity * 0.001f;
                rot.Z = 0;
                Rotation = rot;
            }
        }
        if (@event is InputEventMouseButton button)
        {
            MouseButton mb = button.ButtonIndex;
            if(mb == MouseButton.Left)
            {
                if (button.IsReleased()) view = false;
                else if (button.IsPressed())
                {
                    view = true;
                    reset = true;
                }
            }
        }
    }
}
