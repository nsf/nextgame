[VS]
uniform mat4 ModelViewProj;

in vec3 NG_Position;
out vec3 f_position_world;

void main() {
	f_position_world = NG_Position;
	gl_Position = ModelViewProj * vec4(NG_Position, 1.0);
}

[FS]
const float e = 2.71828182845904523536028747135266249775724709369995957f;
const float pi = 3.141592653589793238462643383279502884197169;
const float rad2deg = 180.0 / pi;

const float n = 1.0003; // refractive index of air
const float N = 2.545E25; // number of molecules per unit volume for air at
						// 288.15K and 1013mb (sea level -45 celsius)
const float pn = 0.035;	// depolatization factor for standard air

// wavelength of used primaries, according to preetham
const vec3 lambda = vec3(680E-9, 550E-9, 450E-9);

const vec3 g_betaR = vec3(5.8e-6, 1.35e-5, 3.31e-5);
const vec3 g_betaMSca = vec3(4e-3);
const vec3 g_betaM = g_betaMSca / 0.9;

// mie stuff
// K coefficient for the primaries
const vec3 K = vec3(0.686f, 0.678f, 0.666f);
const float v = 4.0f;

// optical length at zenith for molecules
const float rayleigh_zenith_length = 8.4E3;
const float mie_zenith_length = 1.25E3;
const vec3 up = vec3(0, 0, 1);

const float E = 1000.0f;
const float sun_angular_diameter_cos = 0.999956676946448443553574619906976478926848692873900859324f;

// earth shadow hack
// const float cutoff_angle = pi/2.0f;
// const float steepness = 0.5f;
const float cutoff_angle = pi/1.95;
const float steepness = 1.5;

vec3 total_rayleigh(vec3 lambda) {
	return (8 * pow(pi, 3) * pow(pow(n, 2) - 1, 2) * (6 + 3 * pn)) /
		(3 * N * pow(lambda, vec3(4)) * (6 - 7 * pn));
}

float rayleigh_phase(float cos_theta) {
	// NOTE: There are a few scale factors for the phase funtion
	// (1) as given bei Preetham, normalized over the sphere with 4pi sr
	// (2) normalized to integral = 1
	// (3) nasa: integrates to 9pi / 4, looks best

	//return (3.0f / (16.0f*pi)) * (1.0f + pow(cos_theta, 2));
	// return (1.0f / (3.0f*pi)) * (1.0f + pow(cos_theta, 2));
	return (3.0f / 4.0f) * (1.0f + pow(cos_theta, 2));
}

vec3 total_mie(vec3 lambda, vec3 K, float T) {
	float c = (0.6544 * T - 0.6510) * 10e-16;
	return 0.434 * c * pi * pow((2 * pi) / lambda, vec3(v - 2)) * K;
}

float hg_phase(float cos_theta, float g) {
	return (1.0f / (4.0f*pi)) *
		((1.0f - pow(g, 2)) / pow(1.0f - 2.0f*g*cos_theta + pow(g, 2), 1.5));
	// return ((3 * (1 - g*g)) / (2 * (2 + g*g))) *
	// 	(1 + pow(cos_theta, 2)) / pow((1 + g*g - 2 * g * cos_theta), 1.5);
}

float sun_intensity(float zenith_angle_cos) {
	return E * max(0, 1 - exp(-((cutoff_angle - acos(zenith_angle_cos))/steepness)));
}

in vec3 f_position_world;

uniform vec3 CameraPosition;
uniform vec3 SunDirection;
uniform float RayleighCoefficient;
uniform float MieCoefficient;
uniform float MieDirectionalG;
uniform float Turbidity;

out vec3 NG_Col0;

void main() {
	float sunE = sun_intensity(dot(SunDirection, up));

	// extinction (absorbtion + out scattering)
	// rayleigh coefficients
	vec3 betaR = g_betaR * RayleighCoefficient;

	// mie coefficients
	vec3 betaM = g_betaM * MieCoefficient;

	// optical length
	// cutoff angle at 90 to avoid singularity in next formula.
	float zenith_angle = acos(max(0, dot(up, normalize(f_position_world))));
	float sR = rayleigh_zenith_length /
		(cos(zenith_angle) + 0.15 * pow(93.885 - zenith_angle * rad2deg, -1.253));
	float sM = mie_zenith_length /
		 (cos(zenith_angle) + 0.15 * pow(93.885 - zenith_angle * rad2deg, -1.253));

	// combined extinction factor
	vec3 Fex = exp(-(betaR * sR + betaM * sM));

	// in scattering
	float cos_theta = dot(normalize(f_position_world - CameraPosition), SunDirection);

	float r_phase = rayleigh_phase(cos_theta);
	vec3 betaR_theta = betaR * r_phase;

	float m_phase = hg_phase(cos_theta, MieDirectionalG);
	vec3 betaM_theta = betaM * m_phase;

	vec3 Lin = sunE * ((betaR_theta + betaM_theta) / (betaR + betaM)) * (1 - Fex);
	vec3 L0 = vec3(0);

	// composition + solar disc
	if (cos_theta > sun_angular_diameter_cos)
		L0 += sunE * Fex;

	NG_Col0 = (L0 + Lin)/50;
}