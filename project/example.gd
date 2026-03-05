@tool

extends Node

func _ready() -> void:
	print("JWDebugDraw test!")

func _process(_delta: float) -> void:
	JWDebugDraw.draw_point(Vector3(0,0,0), 0.2)
