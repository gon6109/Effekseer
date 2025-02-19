﻿#define RAW_HSV

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Effekseer.GUI.Component
{
	class ColorWithRandom : Control, IParameterControl
	{
		string id1 = "";
		string id2 = "";
		string id_c = "";
		string id_r1 = "";
		string id_r2 = "";

		bool isPopupShown = false;

		public string Label { get; set; } = string.Empty;

		public string Description { get; set; } = string.Empty;

		Data.Value.ColorWithRandom binding = null;

		ValueChangingProperty valueChangingProp = new ValueChangingProperty();

		bool isActive = false;
		bool isWriting = false;

		float[] internalValueMax = new float[] { 1.0f, 1.0f, 1.0f, 1.0f };
		float[] internalValueMin = new float[] { 1.0f, 1.0f, 1.0f, 1.0f };

		/// <summary>
		/// This parameter is unused.
		/// </summary>
		public bool EnableUndo { get; set; } = true;

		public Data.Value.ColorWithRandom Binding
		{
			get
			{
				return binding;
			}
			set
			{
				if (binding == value) return;

				if(binding != null)
				{
					binding.OnChangedColorSpace -= Binding_OnChangedColorSpace;
					binding.R.OnChanged -= Binding_OnChanged;
					binding.G.OnChanged -= Binding_OnChanged;
					binding.B.OnChanged -= Binding_OnChanged;
					binding.A.OnChanged -= Binding_OnChanged;
				}

				binding = value;

				if (binding != null)
				{
					// Force to minmax
					binding.R.DrawnAs = Data.DrawnAs.MaxAndMin;
					binding.G.DrawnAs = Data.DrawnAs.MaxAndMin;
					binding.B.DrawnAs = Data.DrawnAs.MaxAndMin;
					binding.A.DrawnAs = Data.DrawnAs.MaxAndMin;

					internalValueMax[0] = binding.R.Max / 255.0f;
					internalValueMax[1] = binding.G.Max / 255.0f;
					internalValueMax[2] = binding.B.Max / 255.0f;
					internalValueMax[3] = binding.A.Max / 255.0f;

					internalValueMin[0] = binding.R.Min / 255.0f;
					internalValueMin[1] = binding.G.Min / 255.0f;
					internalValueMin[2] = binding.B.Min / 255.0f;
					internalValueMin[3] = binding.A.Min / 255.0f;

					binding.OnChangedColorSpace += Binding_OnChangedColorSpace;
					binding.R.OnChanged += Binding_OnChanged;
					binding.G.OnChanged += Binding_OnChanged;
					binding.B.OnChanged += Binding_OnChanged;
					binding.A.OnChanged += Binding_OnChanged;
				}
			}
		}


		public ColorWithRandom(string label = null)
		{
			if (label != null)
			{
				Label = label;
			}

			id1 = "###" + Manager.GetUniqueID().ToString();
			id2 = "###" + Manager.GetUniqueID().ToString();
			id_c = "###" + Manager.GetUniqueID().ToString();
			id_r1 = "###" + Manager.GetUniqueID().ToString();
			id_r2 = "###" + Manager.GetUniqueID().ToString();
		}

		public void SetBinding(object o)
		{
			var o_ = o as Data.Value.ColorWithRandom;
			Binding = o_;
		}

		public void FixValue()
		{
			binding.SetMin(
				(int)Math.Round(internalValueMin[0] * 255, MidpointRounding.AwayFromZero),
				(int)Math.Round(internalValueMin[1] * 255, MidpointRounding.AwayFromZero),
				(int)Math.Round(internalValueMin[2] * 255, MidpointRounding.AwayFromZero),
				(int)Math.Round(internalValueMin[3] * 255, MidpointRounding.AwayFromZero),
				isActive);

			binding.SetMax(
				(int)Math.Round(internalValueMax[0] * 255, MidpointRounding.AwayFromZero),
				(int)Math.Round(internalValueMax[1] * 255, MidpointRounding.AwayFromZero),
				(int)Math.Round(internalValueMax[2] * 255, MidpointRounding.AwayFromZero),
				(int)Math.Round(internalValueMax[3] * 255, MidpointRounding.AwayFromZero),
				isActive);
		}

		public override void Update()
		{
			if (binding == null) return;

			valueChangingProp.Enable(binding);

			isPopupShown = false;

			var colorSpace = binding.ColorSpace == Data.ColorSpace.RGBA ? swig.ColorEditFlags.RGB : swig.ColorEditFlags.HSV;
			
			Manager.NativeManager.PushItemWidth(Manager.NativeManager.GetColumnWidth() - 60);

			if (Manager.NativeManager.ColorEdit4(id1, internalValueMin, swig.ColorEditFlags.NoOptions | colorSpace))
			{
				isWriting = true;

				FixValue();

				isWriting = false;
			}


			var isActive_Current = Manager.NativeManager.IsItemActive();

			Popup();

			Manager.NativeManager.SameLine();
			Manager.NativeManager.Text(Resources.GetString("Min"));

			if (Manager.NativeManager.ColorEdit4(id2, internalValueMax, swig.ColorEditFlags.NoOptions | colorSpace))
			{
				isWriting = true;

				FixValue();

				isWriting = false;
			}


			isActive_Current = isActive_Current || Manager.NativeManager.IsItemActive();

			if (isActive && !isActive_Current)
			{
				FixValue();
			}

			isActive = isActive_Current;

			Popup();

			Manager.NativeManager.SameLine();
			Manager.NativeManager.Text(Resources.GetString("Max"));

			Manager.NativeManager.PopItemWidth();

			valueChangingProp.Disable();
		}

		/// <summary>
		/// Show popup
		/// </summary>
		void Popup()
		{
			if (isPopupShown) return;

			if (Manager.NativeManager.BeginPopupContextItem(id_c))
			{
				var txt_r_r1 = "RGBA";
				var txt_r_r2 = "HSVA";

				if (Manager.NativeManager.RadioButton(txt_r_r1 + id_r1, binding.ColorSpace == Data.ColorSpace.RGBA))
				{
					binding.ChangeColorSpace(Data.ColorSpace.RGBA, true);
				}

				Manager.NativeManager.SameLine();

				if (Manager.NativeManager.RadioButton(txt_r_r2 + id_r2, binding.ColorSpace == Data.ColorSpace.HSVA))
				{
					binding.ChangeColorSpace(Data.ColorSpace.HSVA, true);
				}

				Manager.NativeManager.EndPopup();
				isPopupShown = true;
			}

		}

		private void Binding_OnChangedColorSpace(object sender, ChangedValueEventArgs e)
		{
			if (binding != null)
			{
				internalValueMax[0] = binding.R.Max / 255.0f;
				internalValueMax[1] = binding.G.Max / 255.0f;
				internalValueMax[2] = binding.B.Max / 255.0f;
				internalValueMax[3] = binding.A.Max / 255.0f;

				internalValueMin[0] = binding.R.Min / 255.0f;
				internalValueMin[1] = binding.G.Min / 255.0f;
				internalValueMin[2] = binding.B.Min / 255.0f;
				internalValueMin[3] = binding.A.Min / 255.0f;
			}
		}

		private void Binding_OnChanged(object sender, ChangedValueEventArgs e)
		{
			if (isWriting) return;
			if (binding != null)
			{
				internalValueMax[0] = binding.R.Max / 255.0f;
				internalValueMax[1] = binding.G.Max / 255.0f;
				internalValueMax[2] = binding.B.Max / 255.0f;
				internalValueMax[3] = binding.A.Max / 255.0f;

				internalValueMin[0] = binding.R.Min / 255.0f;
				internalValueMin[1] = binding.G.Min / 255.0f;
				internalValueMin[2] = binding.B.Min / 255.0f;
				internalValueMin[3] = binding.A.Min / 255.0f;
			}
		}

		void convertRGB2HSV(float[] values)
		{
			Effekseer.Utl.RGBHSVColor c = new Effekseer.Utl.RGBHSVColor();
			c.RH = (int)Math.Round(values[0] * 255, MidpointRounding.AwayFromZero);
			c.GS = (int)Math.Round(values[1] * 255, MidpointRounding.AwayFromZero);
			c.BV = (int)Math.Round(values[2] * 255, MidpointRounding.AwayFromZero);
			c = Effekseer.Utl.RGBHSVColor.RGBToHSV(c);
			values[0] = c.RH / 255.0f;
			values[1] = c.GS / 255.0f;
			values[2] = c.BV / 255.0f;
		}

		void convertHSV2RGB(float[] values)
		{
			Effekseer.Utl.RGBHSVColor c = new Effekseer.Utl.RGBHSVColor();
			c.RH = (int)Math.Round(values[0] * 255, MidpointRounding.AwayFromZero);
			c.GS = (int)Math.Round(values[1] * 255, MidpointRounding.AwayFromZero);
			c.BV = (int)Math.Round(values[2] * 255, MidpointRounding.AwayFromZero);
			c = Effekseer.Utl.RGBHSVColor.HSVToRGB(c);
			values[0] = c.RH / 255.0f;
			values[1] = c.GS / 255.0f;
			values[2] = c.BV / 255.0f;
		}
	}
}
