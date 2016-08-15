# include <Siv3D.hpp>

Vec2 GetBorder(const Vec2& start, const Vec2& direction, const Sphere& sphere)
{
	double min = 0.0, max = 2048.0;

	Vec2 pos(0, 0);

	while((max - min) >= 0.001)
	{
		const double mid = (min + max) / 2;

		pos = start + direction * mid;

		if (Graphics3D::ToRay(pos).intersectsAt(sphere))
		{
			min = mid;
		}
		else
		{
			max = mid;
		}
	}

	return start + direction * min;
}

void Main()
{
	Window::Resize(1280, 720);
	Window::SetTitle(L"11x11x11 Blocks");
	Graphics::SetBackground(Color(190, 240, 210));
	Graphics3D::SetAmbientLight(ColorF(0.3));
	Graphics3D::SetCamera(Camera(Vec3(-5, 12, -24), Vec3(-5, 0, 0), Vec3::Up, 45, 1));

	const Sphere sphere(5.5 * Sqrt(3));
	const int32 tileSize = 30;

	const Grid<Color> palette = 
	{
		{ HSV(0), HSV(30), HSV(60), HSV(90), HSV(120) },
		{ HSV(150), HSV(180), HSV(210), HSV(240), HSV(270) },
		{ HSV(300), HSV(330), Color(255), Color(160), Color(40) },
	};

	const Array<Point> pairs =
	{
		{ 0,1 },{ 1,3 },{ 3,2 },{ 2,0 },
		{ 0,4 },{ 1,5 },{ 3,7 },{ 2,6 },
		{ 4,5 },{ 5,7 },{ 7,6 },{ 6,4 },
	};

	std::array<std::array<std::array<Optional<Color>, 11>, 11>, 11> blocks;

	Quaternion rotation, rotation2;
	Optional<Point> grabbed;
	Vec3 from(0, 0, 0), to(0, 0, 0);
	Optional<int32> yLayer = 10, zLayer = none;
	Color penColor = palette[{0, 1}];

	while (System::Update())
	{
		const double a = (Sin(Time::GetMillisec() % 3000 / 3000.0 * TwoPi) + 1.0) * 0.15 + 0.1;

		const auto onSphere = Mouse::Ray().intersectsAt(sphere);

		Cursor::SetStyle(onSphere ? CursorStyle::Hand : CursorStyle::Default);

		if (onSphere && Input::MouseL.clicked)
		{
			grabbed = Mouse::Pos();

			from = to = onSphere.value();
		}

		if (grabbed)
		{
			if (onSphere)
			{
				to = onSphere.value();

				rotation2 = Quaternion::RotationArc(from.normalized(), to.normalized());
			}
			else if (Mouse::Pos() != grabbed)
			{
				const Vec2 pos = GetBorder(grabbed.value(), Vec2(Mouse::Pos() - grabbed.value()).normalized(), sphere);

				const double s = Mouse::Pos().distanceFrom(grabbed.value()) / grabbed->distanceFrom(pos);

				to = Graphics3D::ToRay(pos).intersectsAt(sphere).value();

				rotation2 = Slerp(Quaternion::Identity(), Quaternion::RotationArc(from.normalized(), to.normalized()), s);
			}
		}

		if(Input::MouseL.released)
		{
			grabbed = none;

			rotation *= rotation2;

			rotation2 = Quaternion::Identity();
		}

		const Quaternion rot = rotation * rotation2;

		for (auto y : step(11))
		{
			for (auto z : step(11))
			{
				for (auto x : step(11))
				{
					if (const auto color = blocks[y][z][x])
					{
						Box(1).asMesh().translated(x - 5, 5 - y, 5 - z).rotated(rot).draw(color.value());
					}
				}
			}
		}

		for (auto i : step(11))
		{
			for (auto x : step(11))
			{
				auto& block = yLayer ? blocks[yLayer.value()][i][x] : blocks[i][zLayer.value()][x];

				const Rect rect(40 + x * tileSize, 50 + i * tileSize, tileSize);

				if (block)
				{
					rect.draw(block.value());
				}

				if (yLayer && yLayer < 10)
				{
					const auto under = blocks[yLayer.value() + 1][i][x];

					if (under)
					{
						rect.movedBy(-2, 2).draw(ColorF(under.value(), a));
					}
				}
				else if (zLayer && zLayer > 0)
				{
					const auto under = blocks[i][zLayer.value() - 1][x];

					if (under)
					{
						rect.movedBy(-2, 2).draw(ColorF(under.value(), a));
					}
				}

				rect.drawFrame(0.5, 0, Color(127));

				if (!grabbed && rect.leftPressed)
				{
					block = penColor;
				}

				if (!grabbed && rect.rightPressed)
				{
					block = none;
				}
			}
		}

		for (auto i : step(11))
		{
			const Circle circle(400, 50 + tileSize / 2 + tileSize * i, tileSize * 0.4);

			if (i == yLayer)
			{
				circle.draw(Palette::White);
			}

			circle.drawFrame(1, 0, Color(127));

			if (circle.leftClicked)
			{
				zLayer = none;
				yLayer = i;
			}
		}

		for (auto i : step(11))
		{
			const Circle circle(40 + tileSize / 2 + tileSize * (10 - i), 410, tileSize * 0.4);

			if ((10 - i) == zLayer)
			{
				circle.draw(Palette::White);
			}

			circle.drawFrame(1, 0, Color(127));

			if (circle.leftClicked)
			{
				yLayer = none;
				zLayer = 10 - i;
			}
		}

		const Array<Vec3> pts =
		{
			rot * Vec3(-5.5,  5.5,  5.5), rot * Vec3( 5.5,  5.5,  5.5),
			rot * Vec3(-5.5,  5.5, -5.5), rot * Vec3( 5.5,  5.5, -5.5),
			rot * Vec3(-5.5, -5.5,  5.5), rot * Vec3( 5.5, -5.5,  5.5),
			rot * Vec3(-5.5, -5.5, -5.5), rot * Vec3( 5.5, -5.5, -5.5),
		};

		for (const auto& pair : pairs)
		{
			Line3D(pts[pair.x], pts[pair.y]).drawForward(AlphaF(a * 2));
		}

		for (auto p : step(palette.size()))
		{
			Rect rect(Point(40, 450) + p * 65, 65);

			if (penColor == palette[p])
			{
				rect = rect.stretched(-5);
			}

			rect.draw(palette[p]).drawFrame(1, 0, Color(0, 80));

			if (rect.leftClicked)
			{
				penColor = palette[p];
			}
		}
	}
}
