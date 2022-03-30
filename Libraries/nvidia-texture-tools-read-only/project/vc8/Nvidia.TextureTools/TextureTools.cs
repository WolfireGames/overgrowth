using System;
using System.Security;
using System.Runtime.InteropServices;

namespace Nvidia.TextureTools
{
	#region Enums

	#region public enum Format
	/// <summary>
	/// Compression format.
	/// </summary>
	public enum Format
	{
		// No compression.
		RGB,
		RGBA = RGB,

		// DX9 formats.
		DXT1,
		DXT1a,
		DXT3,
		DXT5,
		DXT5n,

		// DX10 formats.
		BC1 = DXT1,
		BC1a = DXT1a,
		BC2 = DXT3,
		BC3 = DXT5,
		BC3n = DXT5n,
		BC4,
		BC5,
	}
	#endregion

	#region public enum Quality
	/// <summary>
	/// Quality modes.
	/// </summary>
	public enum Quality
	{
		Fastest,
		Normal,
		Production,
		Highest,
	}
	#endregion

	#region public enum WrapMode
	/// <summary>
	/// Wrap modes.
	/// </summary>
	public enum WrapMode
	{
		Clamp,
		Repeat,
		Mirror,
	}
	#endregion

	#region public enum TextureType
	/// <summary>
	/// Texture types.
	/// </summary>
	public enum TextureType
	{
		Texture2D,
		TextureCube,
	}
	#endregion

	#region public enum InputFormat
	/// <summary>
	/// Input formats.
	/// </summary>
	public enum InputFormat
	{
		BGRA_8UB
	}
	#endregion

	#region public enum MipmapFilter
	/// <summary>
	/// Mipmap downsampling filters.
	/// </summary>
	public enum MipmapFilter
	{
		Box,
		Triangle,
		Kaiser
	}
	#endregion

	#region public enum ColorTransform
	/// <summary>
	/// Color transformation.
	/// </summary>
	public enum ColorTransform
	{
		None,
		Linear
	}
	#endregion

	#region public enum RoundMode
	/// <summary>
	/// Extents rounding mode.
	/// </summary>
	public enum RoundMode
	{
		None,
		ToNextPowerOfTwo,
		ToNearestPowerOfTwo,
		ToPreviousPowerOfTwo
	}
	#endregion

	#region public enum AlphaMode
	/// <summary>
	/// Alpha mode.
	/// </summary>
	public enum AlphaMode
	{
		None,
		Transparency,
		Premultiplied
	}
	#endregion

	#region public enum Error
	/// <summary>
	/// Error codes.
	/// </summary>
	public enum Error
	{
		InvalidInput,
		UserInterruption,
		UnsupportedFeature,
		CudaError,
		Unknown,
		FileOpen,
		FileWrite,
	}
	#endregion

	#endregion

	#region public class InputOptions
	/// <summary>
	/// Input options.
	/// </summary>
	public class InputOptions
	{
		#region Bindings
		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static IntPtr nvttCreateInputOptions();

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttDestroyInputOptions(IntPtr inputOptions);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsTextureLayout(IntPtr inputOptions, TextureType type, int w, int h, int d);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttResetInputOptionsTextureLayout(IntPtr inputOptions);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static bool nvttSetInputOptionsMipmapData(IntPtr inputOptions, IntPtr data, int w, int h, int d, int face, int mipmap);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsFormat(IntPtr inputOptions, InputFormat format);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsAlphaMode(IntPtr inputOptions, AlphaMode alphaMode);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsGamma(IntPtr inputOptions, float inputGamma, float outputGamma);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsWrapMode(IntPtr inputOptions, WrapMode mode);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsMipmapFilter(IntPtr inputOptions, MipmapFilter filter);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsMipmapGeneration(IntPtr inputOptions, bool generateMipmaps, int maxLevel);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsKaiserParameters(IntPtr inputOptions, float width, float alpha, float stretch);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsNormalMap(IntPtr inputOptions, bool b);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsConvertToNormalMap(IntPtr inputOptions, bool convert);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsHeightEvaluation(IntPtr inputOptions, float redScale, float greenScale, float blueScale, float alphaScale);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsNormalFilter(IntPtr inputOptions, float small, float medium, float big, float large);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsNormalizeMipmaps(IntPtr inputOptions, bool b);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsColorTransform(IntPtr inputOptions, ColorTransform t);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsLinearTransfrom(IntPtr inputOptions, int channel, float w0, float w1, float w2, float w3);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsMaxExtents(IntPtr inputOptions, int d);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetInputOptionsRoundMode(IntPtr inputOptions, RoundMode mode);
		#endregion

		internal IntPtr options;

		public InputOptions()
		{
			options = nvttCreateInputOptions();
		}
		~InputOptions()
		{
			nvttDestroyInputOptions(options);
		}

		public void SetTextureLayout(TextureType type, int w, int h, int d)
		{
			nvttSetInputOptionsTextureLayout(options, type, w, h, d);
		}
		public void ResetTextureLayout()
		{
			nvttResetInputOptionsTextureLayout(options);
		}

		public void SetMipmapData(IntPtr data, int width, int height, int depth, int face, int mipmap)
		{
			nvttSetInputOptionsMipmapData(options, data, width, height, depth, face, mipmap);
		}

		public void SetFormat(InputFormat format)
		{
			nvttSetInputOptionsFormat(options, format);
		}

		public void SetAlphaMode(AlphaMode alphaMode)
		{
			nvttSetInputOptionsAlphaMode(options, alphaMode);
		}

		public void SetGamma(float inputGamma, float outputGamma)
		{
			nvttSetInputOptionsGamma(options, inputGamma, outputGamma);
		}

		public void SetWrapMode(WrapMode wrapMode)
		{
			nvttSetInputOptionsWrapMode(options, wrapMode);
		}

		public void SetMipmapFilter(MipmapFilter filter)
		{
			nvttSetInputOptionsMipmapFilter(options, filter);
		}

		public void SetMipmapGeneration(bool enabled)
		{
			nvttSetInputOptionsMipmapGeneration(options, enabled, -1);
		}

		public void SetMipmapGeneration(bool enabled, int maxLevel)
		{
			nvttSetInputOptionsMipmapGeneration(options, enabled, maxLevel);
		}

		public void SetKaiserParameters(float width, float alpha, float stretch)
		{
			nvttSetInputOptionsKaiserParameters(options, width, alpha, stretch);
		}

		public void SetNormalMap(bool b)
		{
			nvttSetInputOptionsNormalMap(options, b);
		}

		public void SetConvertToNormalMap(bool convert)
		{
			nvttSetInputOptionsConvertToNormalMap(options, convert);
		}

		public void SetHeightEvaluation(float redScale, float greenScale, float blueScale, float alphaScale)
		{
			nvttSetInputOptionsHeightEvaluation(options, redScale, greenScale, blueScale, alphaScale);
		}

		public void SetNormalFilter(float small, float medium, float big, float large)
		{
			nvttSetInputOptionsNormalFilter(options, small, medium, big, large);
		}

		public void SetNormalizeMipmaps(bool b)
		{
			nvttSetInputOptionsNormalizeMipmaps(options, b);
		}

		public void SetColorTransform(ColorTransform t)
		{
			nvttSetInputOptionsColorTransform(options, t);
		}

		public void SetLinearTransfrom(int channel, float w0, float w1, float w2, float w3)
		{
			nvttSetInputOptionsLinearTransfrom(options, channel, w0, w1, w2, w3);
		}

		public void SetMaxExtents(int dim)
		{
			nvttSetInputOptionsMaxExtents(options, dim);
		}

		public void SetRoundMode(RoundMode mode)
		{
			nvttSetInputOptionsRoundMode(options, mode);
		}
	}
	#endregion

	#region public class CompressionOptions
	/// <summary>
	/// Compression options.
	/// </summary>
	public class CompressionOptions
	{
		#region Bindings
		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static IntPtr nvttCreateCompressionOptions();

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttDestroyCompressionOptions(IntPtr compressionOptions);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetCompressionOptionsFormat(IntPtr compressionOptions, Format format);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetCompressionOptionsQuality(IntPtr compressionOptions, Quality quality);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetCompressionOptionsColorWeights(IntPtr compressionOptions, float red, float green, float blue, float alpha);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetCompressionOptionsPixelFormat(IntPtr compressionOptions, uint bitcount, uint rmask, uint gmask, uint bmask, uint amask);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetCompressionOptionsQuantization(IntPtr compressionOptions, bool colorDithering, bool alphaDithering, bool binaryAlpha, int alphaThreshold);
		#endregion

		internal IntPtr options;

		public CompressionOptions()
		{
			options = nvttCreateCompressionOptions();
		}
		~CompressionOptions()
		{
			nvttDestroyCompressionOptions(options);
		}

		public void SetFormat(Format format)
		{
			nvttSetCompressionOptionsFormat(options, format);
		}
		
		public void SetQuality(Quality quality)
		{
			nvttSetCompressionOptionsQuality(options, quality);
		}

		public void SetColorWeights(float red, float green, float blue)
		{
			nvttSetCompressionOptionsColorWeights(options, red, green, blue, 1.0f);
		}

		public void SetColorWeights(float red, float green, float blue, float alpha)
		{
			nvttSetCompressionOptionsColorWeights(options, red, green, blue, alpha);
		}

		public void SetPixelFormat(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask)
		{
			nvttSetCompressionOptionsPixelFormat(options, bitcount, rmask, gmask, bmask, amask);
		}

		public void SetQuantization(bool colorDithering, bool alphaDithering, bool binaryAlpha)
		{
			nvttSetCompressionOptionsQuantization(options, colorDithering, alphaDithering, binaryAlpha, 127);
		}

		public void SetQuantization(bool colorDithering, bool alphaDithering, bool binaryAlpha, int alphaThreshold)
		{
			nvttSetCompressionOptionsQuantization(options, colorDithering, alphaDithering, binaryAlpha, alphaThreshold);
		}
	}
	#endregion

	#region public class OutputOptions
	/// <summary>
	/// Output options.
	/// </summary>
	public class OutputOptions
	{
		#region Delegates
		public delegate void ErrorHandler(Error error);
		private delegate void WriteDataDelegate(IntPtr data, int size);
		private delegate void ImageDelegate(int size, int width, int height, int depth, int face, int miplevel);
		#endregion

		#region Bindings
		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static IntPtr nvttCreateOutputOptions();

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttDestroyOutputOptions(IntPtr outputOptions);

		[DllImport("nvtt", CharSet = CharSet.Ansi), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetOutputOptionsFileName(IntPtr outputOptions, string fileName);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetOutputOptionsErrorHandler(IntPtr outputOptions, ErrorHandler errorHandler);

		private void ErrorCallback(Error error)
		{
			if (Error != null) Error(error);
		}

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttSetOutputOptionsOutputHeader(IntPtr outputOptions, bool b);

		//[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		//private extern static void nvttSetOutputOptionsOutputHandler(IntPtr outputOptions, WriteDataDelegate writeData, ImageDelegate image);

		#endregion

		internal IntPtr options;

		public OutputOptions()
		{
			options = nvttCreateOutputOptions();
			nvttSetOutputOptionsErrorHandler(options, new ErrorHandler(ErrorCallback));
		}
		~OutputOptions()
		{
			nvttDestroyOutputOptions(options);
		}

		public void SetFileName(string fileName)
		{
			nvttSetOutputOptionsFileName(options, fileName);
		}

		public event ErrorHandler Error;

		public void SetOutputHeader(bool b)
		{
			nvttSetOutputOptionsOutputHeader(options, b);
		}

		// @@ Add OutputHandler interface.
	}
	#endregion

	#region public static class Compressor
	public class Compressor
	{
		#region Bindings
		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static IntPtr nvttCreateCompressor();

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static void nvttDestroyCompressor(IntPtr compressor);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static bool nvttCompress(IntPtr compressor, IntPtr inputOptions, IntPtr compressionOptions, IntPtr outputOptions);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private extern static int nvttEstimateSize(IntPtr compressor, IntPtr inputOptions, IntPtr compressionOptions);

		[DllImport("nvtt"), SuppressUnmanagedCodeSecurity]
		private static extern IntPtr nvttErrorString(Error error);

		#endregion

		internal IntPtr compressor;

		public Compressor()
		{
			compressor = nvttCreateCompressor();
		}

		~Compressor()
		{
			nvttDestroyCompressor(compressor);
		}

		public bool Compress(InputOptions input, CompressionOptions compression, OutputOptions output)
		{
			return nvttCompress(compressor, input.options, compression.options, output.options);
		}

		public int EstimateSize(InputOptions input, CompressionOptions compression)
		{
			return nvttEstimateSize(compressor, input.options, compression.options);
		}

		public static string ErrorString(Error error)
		{
			return Marshal.PtrToStringAnsi(nvttErrorString(error));
		}

	}
	#endregion

} // Nvidia.TextureTools namespace
